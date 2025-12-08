#pragma once
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class LibraryManager {
public:
	LibraryManager();

	std::vector<fs::path> mSongs;
	bool import();
	bool erase(const fs::path& song);

private:
	const char* mFilters[3] = { "*.mp3", "*.wav", "*.ogg" };
	fs::path mMusicDir{};
};