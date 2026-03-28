#include "Downloader.h"
#include "Utils.h"
#include <print>
#include <fstream>

Downloader::Downloader() : mMainDir(utils::getBasePath() / "AudioPlayer"), mYtDlpDir(mMainDir / "yt-dlp")
{
	utils::createDirectory(mMainDir);
	utils::createDirectory(mYtDlpDir);
	extractYtDlp();
}

bool Downloader::download(const std::string& url, const fs::path& musicDir) {
	if (getDownloadStatus() == DownloadStatus::Downloading) return false;

	const std::string outputTemplate = (musicDir / "%(title)s.%(ext)s").string();

#if defined(_WIN32)
	std::string cmd = "\"" + mYtDlpPath.string() + "\" -x --audio-format mp3 --audio-quality 0 --windows-filenames -o \"" + outputTemplate + "\" " + url;
	std::wstring wCmd = toWide(cmd);

	STARTUPINFOW si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};

	const BOOL success = CreateProcessW(nullptr, wCmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

	if (!success) {
		if (pi.hProcess) CloseHandle(pi.hProcess);
		if (pi.hThread) CloseHandle(pi.hThread);
		std::println("Failed to launch yt-dlp process. Error code: {}\n", GetLastError());
		mDownloadStatus = DownloadStatus::Failed;
		return false;
	}

	mProcessHandle = pi.hProcess;
	mThreadHandle = pi.hThread;
	mDownloadStatus = DownloadStatus::Downloading;
	return true;
#else
	pid_t pid = fork();

	if (pid < 0) {
		std::println("fork() failed");
		mDownloadStatus = DownloadStatus::Failed;
		return false;
	}

	if (pid == 0) {
		execl(mYtDlpPath.c_str(),
			"yt-dlp", "-x", "--audio-format", "mp3", "--audio-quality", "0", "--windows-filenames", "--ffmpeg-location", mYtDlpDir.c_str(), "-o",
			outputTemplate.c_str(), url.c_str(), nullptr);
		std::println("execl(), failed - ut - dlp could not be launcehd\n");
		_exit(1);
	}

	mProcessId = pid;
	mDownloadStatus = DownloadStatus::Downloading;
	return true;
#endif
}

Downloader::DownloadStatus Downloader::getDownloadStatus() {
	if (mDownloadStatus != DownloadStatus::Downloading) return mDownloadStatus;

#if defined(_WIN32)
	if (mProcessHandle == nullptr) return DownloadStatus::Idle;
	DWORD exitCode;
	GetExitCodeProcess(mProcessHandle, &exitCode);
	if (exitCode == STILL_ACTIVE) {
		return DownloadStatus::Downloading;
	}
	CloseHandle(mProcessHandle);
	CloseHandle(mThreadHandle);
	mProcessHandle = nullptr;
	mThreadHandle = nullptr;
	const DownloadStatus result = (exitCode == 0) ? DownloadStatus::Success : DownloadStatus::Failed;
	mDownloadStatus = DownloadStatus::Idle;
	return result;
#else
	if (mProcessId < 0) return DownloadStatus::Idle;
	int status;
	const pid_t resilt = waitpid(mProcessId, &status, WNOHANG);
	if (resilt == 0) {
		return DownloadStatus::Downloading;
	}
	mProcessId = -1;
	const DownloadStatus result = (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		? DownloadStatus::Success
		: DownloadStatus::Failed;
	mDownloadStatus = DownloadStatus::Idle;
	return result;
#endif
}

bool Downloader::isReady() const {
	return fs::exists(mYtDlpPath) && fs::exists(mFfmpegPath);
}

bool Downloader::extractYtDlp() {
#if defined(_WIN32)
	mYtDlpPath = mYtDlpDir / "yt-dlp.exe";
	mFfmpegPath = mYtDlpDir / "ffmpeg.exe";
	const std::string ytDlpResource = "vendors/yt-dlp/yt-dlp.exe";
	const std::string ffmpegResource = "vendors/yt-dlp/ffmpeg.exe";
#elif defined(__APPLE__)
	mYtDlpPath = mYtDlpDir / "yt-dlp_macos";
	mFfmpegPath = mYtDlpDir / "ffmpeg";
	const std::string ytDlpResource = "vendors/yt-dlp/yt-dlp_macos";
	const std::string ffmpegResource = "vendors/yt-dlp/ffmpeg_macos";
#else
	mYtDlpPath = mYtDlpDir / "yt-dlp";
	mFfmpegPath = mYtDlpDir / "ffmpeg";
	const std::string ytDlpResource = "vendors/yt-dlp/yt-dlp";
	const std::string ffmpegResource = "vendors/yt-dlp/ffmpeg";
#endif

	const bool ytDlpOk = extractBinary(ytDlpResource, mYtDlpPath);
	const bool ffmpegOk = extractBinary(ffmpegResource, mFfmpegPath);
	return ytDlpOk && ffmpegOk;
}

bool Downloader::extractBinary(const std::string& resourcePath, const fs::path& outputPath) {
	if (fs::exists(outputPath)) return true;
	const auto fileSystem = cmrc::dlp::get_filesystem();

	if (!fileSystem.exists(resourcePath)) {
		std::println("Resource not found in embedded filesystem: {}\n", resourcePath);
		return false;
	}

	const auto file = fileSystem.open(resourcePath);

	std::ofstream out(outputPath, std::ios::binary);
	if (!out.is_open()) {
		std::println("Failed to open output file for extraction: {}\n", outputPath.string());
		return false;
	}

	out.write(reinterpret_cast<const char*>(file.begin()), file.size());
	out.close();

#if !defined(_WIN32)
	std::error_code ec;
	fs::permissions(outputPath,
		fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
		fs::perm_options::add, ec);

	if (ec) {
		std::println("Failed to set executable permissions on {}: {}\n", outputPath.string(), ec.message());
		return false;
	}
#endif

	std::println("Extracted successfully: {}\n", outputPath.string());
	return true;
}

#if defined(_WIN32)
std::wstring Downloader::toWide(const std::string& str) {
	if (str.empty()) return {};
	const int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	std::wstring result(size - 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
	return result;
}
#endif