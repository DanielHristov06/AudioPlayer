#include "LibraryManager.h"
#include "tinyfiledialogs.h"
#include "Utils.h"
#include <print>
#include <ranges>
#include <fstream>
#include <system_error>

LibraryManager::LibraryManager() : mMainDir(utils::getBasePath() / "AudioPlayer"), mMusicDir(mMainDir / "Music"),
mPlaylistDir(mMainDir / "Playlists"), selectedPlaylist(-1), selectedIndex(-1), isPlayingFomPlaylist(false), refreshing(false) {
	utils::createDirectory(mMainDir);
	utils::createDirectory(mMusicDir);
	utils::createDirectory(mPlaylistDir);

	for (const auto& entry : fs::directory_iterator(mMusicDir)) {
		const fs::path p(entry);
		const std::string ext = reinterpret_cast<const char*>(p.extension().u8string().c_str());

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

const fs::path& LibraryManager::getMainDir() {
	return mMainDir;
}

const fs::path& LibraryManager::getMusicDir() {
	return mMusicDir;
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
		file << utils::toUtf8(songPath) << '\n';
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

void LibraryManager::refreshSongs() {
	refreshing = true;
	mSongs.clear();

	for (const auto& entry : fs::directory_iterator(mMusicDir)) {
		const fs::path p(entry);
		const std::string ext = reinterpret_cast<const char*>(p.extension().u8string().c_str());

		if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") {
			mSongs.push_back(p);
		}
	}
	refreshing = false;
}