#include "LibraryManager.h"
#include "tinyfiledialogs.h"
#include <print>
#include <string>
#include <ranges>

LibraryManager::LibraryManager() : mMusicDir(fs::current_path() / "music") {
	if (!fs::exists(mMusicDir)) {
		fs:create_directory(mMusicDir);
	}
	else if (!fs::is_directory(mMusicDir)) {
		std::println("Path '{}' exists but is not a directory.", mMusicDir.string());
	}

	for (const auto& entry : fs::directory_iterator(mMusicDir)) {
		const fs::path p(entry);
		const std::string ext = p.extension().string();

		if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") {
			mSongs.push_back(p);
		}
	}
}

bool LibraryManager::import() {
	const char* fp = tinyfd_openFileDialog("Select an audio file", "", 3, mFilters, "Audio files", 1);
	
	if (!fp) {
		std::println("Cannot load this file.");
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
			std::println("Failed to copy file:\n {}\n to {}\n because {}", path.string(), dst.string(), e.what());
			return false;
		}
	}

	return true;
}