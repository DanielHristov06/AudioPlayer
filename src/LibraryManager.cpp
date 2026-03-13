#include "LibraryManager.h"
#include "tinyfiledialogs.h"
#include <print>
#include <ranges>
#include <fstream>
#include <system_error>

LibraryManager::LibraryManager() : mMainDir(getBasePath() / "AudioPlayer"), mMusicDir(mMainDir / "Music"),
mPlaylistDir(mMainDir / "Playlists"), mYtDlpDir(mMainDir / "yt-dlp"), selectedPlaylist(-1), selectedIndex(-1), isPlayingFomPlaylist(false) {
	createDirectory(mMainDir);
	createDirectory(mMusicDir);
	createDirectory(mPlaylistDir);
	createDirectory(mYtDlpDir);
	extractYtDlp();

	for (const auto& entry : fs::directory_iterator(mMusicDir)) {
		const fs::path p(entry);
		const std::string ext = p.extension().string();

		if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") {
			mSongs.push_back(p);
		}
	}

	for (const auto& entry : fs::directory_iterator(mPlaylistDir)) {
		std::ifstream file(entry.path());
		if (!file.is_open()) continue;
		std::string line;

		std::getline(file, line);
		const std::string name = line.substr(6);

		Playlist currentPlaylist(name, entry.path(), std::vector<fs::path>{});

		while (std::getline(file, line)) {
			const fs::path path(line);

			if (fs::exists(path)) {
				currentPlaylist.songs.push_back(path);
			}
		}

		mPlaylists.push_back(currentPlaylist);
	}
}

bool LibraryManager::import() {
	const char* fp = tinyfd_openFileDialog("Select an audio file", "", 3, mFilters, "Audio files", 1);
	
	if (!fp) {
		std::println("Cannot load this file.\n");
		return false;
	}

	std::vector<fs::path> paths;
	const std::string s(fp);
	auto parts = std::views::split(s, '|');

	for (const auto& part : parts) {
		std::string token;

		for (const char& c : part) {
			token.push_back(c);
		}

		if (!token.empty()) {
			paths.emplace_back(fs::path(token));
		}
	}

	for (const auto& path : paths) {
		const fs::path dst = mMusicDir / path.filename();

		try {
			fs::copy_file(path, dst, fs::copy_options::overwrite_existing);
			if (std::find(mSongs.begin(), mSongs.end(), dst) == mSongs.end()) {
				mSongs.push_back(dst);
			}
		}
		catch (const fs::filesystem_error& e) {
			std::println("Failed to copy file:\n {}\n to {}\n because {}\n", path.string(), dst.string(), e.what());
			return false;
		}
	}

	return true;
}

bool LibraryManager::erase(const fs::path& song) {
	std::error_code ec;
	fs::remove(song, ec);

	if (ec) {
		std::println("Failed to delete file {}\n: {}\n", song.string(), ec.message());
		return false;
	}

	const auto& it = std::find(mSongs.begin(), mSongs.end(), song);

	if (it != mSongs.end()) {
		mSongs.erase(it);
		return true;
	}

	return false;
}

bool LibraryManager::createPlaylist(const std::string& playlist) {
	const fs::path filepath = mPlaylistDir / (playlist + ".plst");

	std::ofstream file(filepath);

	if (!file.is_open()) {
		std::println("Failed to open a playlist file: {}", filepath.string());
		return false;
	}

	file << "Name: " << playlist << '\n';

	mPlaylists.emplace_back(playlist, filepath, std::vector<fs::path>{});

	file.close();

	return true;
}

bool LibraryManager::removePlaylist(Playlist& playlist) {
	const fs::path filepath = playlist.filePath;
	std::error_code ec;
	fs::remove(filepath, ec);

	if (ec) {
		std::println("Failed to delete playlist {}\n: {}\n", filepath.string(), ec.message());
		return false;
	}

	const auto& it = std::find(mPlaylists.begin(), mPlaylists.end(), playlist);

	if (it != mPlaylists.end()) {
		mPlaylists.erase(it);
		return true;
	}

	return false;
}

void LibraryManager::addSongToPlaylist(Playlist& playlist, const fs::path& songPath) const {
	const fs::path filepath = mPlaylistDir / (playlist.name + ".plst");
	std::ofstream file(filepath, std::ios::app);

	if (file.is_open()) {
		file << songPath.string() << '\n';
		playlist.songs.push_back(songPath);
	}
	else {
		std::println("Failed to open playlist for appending: {}", filepath.string());
	}

	file.close();
}

bool LibraryManager::removeSongFromPlaylist(Playlist& playlist, int songIndex) const {
	if (songIndex < 0 || songIndex >= static_cast<int>(playlist.songs.size())) return false;

	playlist.songs.erase(playlist.songs.begin() + songIndex);

	const fs::path filepath = mPlaylistDir / (playlist.name + ".plst");
	std::ofstream file(filepath, std::ios::trunc);
	if (!file.is_open()) {
		std::println("Failed to open playlist for rewriting: {}", filepath.string());
	}

	file << "Name: " << playlist.name << '\n';
	for (const fs::path& song : playlist.songs) {
		file << song.string() << '\n';
	}

	file.close();

	return true;
}

bool LibraryManager::isSongInPlaylist(const fs::path& targetPath, const Playlist& playlist) {
	for (const fs::path& path : playlist.songs) {
		if (path == targetPath) return true;
	}
	return false;
}

bool LibraryManager::download(const std::string& url) {
	if (getDownloadStatus() == DownloadStatus::Downloading) return false;

	const std::string outputTemplate = (mMusicDir / "%(title)s.%(ext)s").string();

#if defined(_WIN32)
	std::string cmd = "\"" + mYtDlpPath.string() + "\" -x --audio-format mp3 --audio-quality 0 -o \"" + outputTemplate + "\" " + url;
	STARTUPINFOA si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};

	const BOOL success = CreateProcessA(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

	if (!success) {
		if (pi.hProcess) CloseHandle(pi.hProcess);
		if (pi.hThread) CloseHandle(pi.hThread);
		std::println("Failed to launch yt-dlp process. Error code: {}", GetLastError());
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
		execl(mYtDlpPath.c_str(), "yt-dlp", "-x", "--audio-format", "mp3", "--audio-quality", "0", "-o", outputTemplate.c_str(), url.c_str(), nullptr);
		std::println("execl(), failed - ut - dlp could not be launcehd");
		_exit(1);
	}

	mProcessId = pid;
	mDownloadStatus = DownloadStatus::Downloading;
	return true;
#endif
}

LibraryManager::DownloadStatus LibraryManager::getDownloadStatus() {
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
	mDownloadStatus = (exitCode == 0) ? DownloadStatus::Success : DownloadStatus::Failed;
	if (mDownloadStatus == DownloadStatus::Success) refreshSongs();
	return mDownloadStatus;
#else
	if (mProcessId < 0) return DownloadStatus::Idle;
	int status;
	const pid_t resilt = waitpid(mProcessId, &status, WNOHANG);
	if (resilt == 0) {
		return DownloadStatus::Downloading;
	}
	mProcessId = -1;
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		mDownloadStatus = DownloadStatus::Success;
		refreshSongs();
	}
	else {
		mDownloadStatus = DownloadStatus::Failed;
	}

	return mDownloadStatus;
#endif
}

void LibraryManager::refreshSongs() {
	mSongs.clear();

	for (const auto& entry : fs::directory_iterator(mMusicDir)) {
		const fs::path p(entry);
		const std::string ext = p.extension().string();

		if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") {
			mSongs.push_back(p);
		}
	}
}

fs::path LibraryManager::getBasePath() {
#if defined(_WIN32)
	const char* appData = std::getenv("APPDATA");
	return appData ? fs::path(appData) : fs::current_path();
#elif defined(__APPLE__)
	const char* home = std::getenv("HOME");
	return home ? fs::path(home) / "Library" / "Application Support" : fs::current_path();
#else
	const char* xdgData = std::getenv("XDG_DATA_HOME");
	if (xdgData) return fs::path(xdgData);

	const char* home = std::getenv("HOME");
	return home ? fs::path(home) / ".local" / "share" : fs::current_path();
#endif
}

bool LibraryManager::extractYtDlp() {
#if defined(_WIN32)
	mYtDlpPath = mYtDlpDir / "yt-dlp.exe";
	const std::string resourcePath = "vendors/yt-dlp/yt-dlp.exe";
#elif defined(__APPLE__)
	mYtDlpPath = mYtDlpDir / "yt-dlp_macos";
	const std::string resourcePath = "vendors/yt-dlp/yt-dlp_macos";
#else
	mYtDlpPath = mYtDlpDir / "yt-dlp";
	const std::string resourcePath = "vendors/yt-dlp/yt-dlp";
#endif

	if (fs::exists(mYtDlpPath)) return true;

	const auto fileSystem = cmrc::dlp::get_filesystem();

	if (!fileSystem.exists(resourcePath)) {
		std::println("yt-dlp was not found in the embedded  system: {}", resourcePath);
		return false;
	}

	const auto file = fileSystem.open(resourcePath);

	std::ofstream out(mYtDlpPath, std::ios::binary);
	if (!out.is_open()) {
		std::println("Failed to open output file for yt-dlp extraction: {}", mYtDlpPath.string());
		return false;
	}

	out.write(reinterpret_cast<const char*>(file.begin()), file.size());
	out.close();

#if !defined(_WIN32)
	std::error_code ec;
	fs::permissions(mYtDlpPath, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add, ec);

	if (ec) {
		std::println("Failed to set executable permissions on yt-dlp: {}", ec.message());
		return false;
	}
#endif

	std::println("yt-dlp extracted succesfully: {}", mYtDlpPath.string());
	return true;
}

void LibraryManager::createDirectory(const fs::path& dir) {
	if (!fs::exists(dir)) {
		fs::create_directory(dir);
	}
	else if (!fs::is_directory(dir)) {
		std::println("Path '{}' exists but is not a directory.\n", dir.string());
	}
}