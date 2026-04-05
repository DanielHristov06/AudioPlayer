#pragma once
#include <filesystem>
#include <string>
#include <cmrc/cmrc.hpp>

#if defined(_WIN32)
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#endif

CMRC_DECLARE(dlp);

namespace fs = std::filesystem;

class Downloader {
public:
	Downloader();

	enum class DownloadStatus { Idle, Downloading, Success, Failed };
	bool download(const std::string& url, const fs::path& musicDir, const std::string& format);
	DownloadStatus getDownloadStatus();
	bool isReady() const;

private:
	fs::path mMainDir{};
	fs::path mYtDlpDir{};
	fs::path mYtDlpPath{};
	fs::path mFfmpegPath{};
	bool extractYtDlp();
	bool extractBinary(const std::string& resourcePath, const fs::path& outputPath);

#if defined(_WIN32)
	HANDLE mProcessHandle = nullptr;
	HANDLE mThreadHandle = nullptr;
	std::wstring toWide(const std::string& str);
#else
	pid_t mProcessId = -1;
#endif

	DownloadStatus mDownloadStatus = DownloadStatus::Idle;
};