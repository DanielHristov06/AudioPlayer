#pragma once
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class LibraryManager {
public:
	LibraryManager();

	std::vector<fs::path> mSongs;
	bool import();

private:
	const char* mFilters[3] = { "*.mp3", "*.wav", "*.ogg" };
	fs::path mMusicDir{};
};