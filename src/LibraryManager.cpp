#include "LibraryManager.h"
#include "tinyfiledialogs.h"
#include "Utils.h"
#include <print>
#include <ranges>
#include <fstream>
#include <system_error>
#include <numeric>
#include <random>

LibraryManager::LibraryManager() : mMainDir(utils::getBasePath() / "AudioPlayer"), mMusicDir(mMainDir / "Music"),
mPlaylistDir(mMainDir / "Playlists"), selectedPlaylist(-1), selectedIndex(-1), mPlayOrderIndex(0), mShuffleEnabled(false), isPlayingFomPlaylist(false), refreshing(false) {
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
		currentPlaylist.buildPlayOrder(currentPlaylist.shuffleEnabled);
	}

	buildPlayOrder(mShuffleEnabled);
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

void LibraryManager::buildPlayOrder(bool shuffle) {
	mPlayOrder.resize(mSongs.size());
	std::iota(mPlayOrder.begin(), mPlayOrder.end(), 0);

	if (shuffle) {
		const int currentSongIndex = getCurrentSongIndex();

		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(mPlayOrder.begin(), mPlayOrder.end(), g);

		if (currentSongIndex >= 0) {
			const auto it = std::find(mPlayOrder.begin(), mPlayOrder.end(), currentSongIndex);
			if (it != mPlayOrder.end()) {
				std::iter_swap(it, mPlayOrder.begin() + mPlayOrderIndex);
			}
		}
	}
	else {
		const int currentSongIndex = getCurrentSongIndex();
		if (currentSongIndex >= 0) mPlayOrderIndex = currentSongIndex;
	}
}

const fs::path& LibraryManager::getMainDir() {
	return mMainDir;
}

const fs::path& LibraryManager::getMusicDir() {
	return mMusicDir;
}

const int LibraryManager::getCurrentSongIndex() const {
	if (mPlayOrderIndex < 0 || mPlayOrderIndex >= mPlayOrder.size()) return -1;
	return mPlayOrder[mPlayOrderIndex];
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
		playlist.buildPlayOrder(playlist.shuffleEnabled);
	}
	else {
		std::println("Failed to open playlist for appending: {}", filepath.string());
	}

	file.close();
}

bool LibraryManager::removeSongFromPlaylist(Playlist& playlist, int songIndex) const {
	if (songIndex < 0 || songIndex >= static_cast<int>(playlist.songs.size())) return false;

	playlist.songs.erase(playlist.songs.begin() + songIndex);
	playlist.buildPlayOrder(playlist.shuffleEnabled);

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

void LibraryManager::setPlaylistsShuffle(bool shuffle) {
	if (mPlaylists.empty()) return;
	for (Playlist& p : mPlaylists) {
		p.shuffleEnabled = shuffle;
		p.buildPlayOrder(p.shuffleEnabled);
	}
}

void LibraryManager::refreshSongs() {
	refreshing = true;

	const int currentIndex = getCurrentSongIndex();
	const fs::path currentPath = (currentIndex >= 0 && currentIndex < static_cast<int>(mSongs.size()))
		? mSongs[currentIndex]
		: fs::path();

	mSongs.clear();

	for (const auto& entry : fs::directory_iterator(mMusicDir)) {
		const fs::path p(entry);
		const std::string ext = reinterpret_cast<const char*>(p.extension().u8string().c_str());

		if (ext == ".mp3" || ext == ".wav" || ext == ".ogg") {
			mSongs.push_back(p);
		}
	}

	buildPlayOrder(mShuffleEnabled);

	if (!currentPath.empty()) {
		const auto it = std::find(mSongs.begin(), mSongs.end(), currentPath);
		if (it != mSongs.end()) {
			const int newSongIndex = static_cast<int>(std::distance(mSongs.begin(), it));
			const auto orderIt = std::find(mPlayOrder.begin(), mPlayOrder.end(), newSongIndex);
			if (orderIt != mPlayOrder.end()) {
				mPlayOrderIndex = static_cast<int>(std::distance(mPlayOrder.begin(), orderIt));
			}
		}
	}

	refreshing = false;
}

void Playlist::buildPlayOrder(bool shuffle) {
	playOrder.resize(songs.size());
	std::iota(playOrder.begin(), playOrder.end(), 0);
	if (shuffle) {
		const int current = (playOrderIndex >= 0 && playOrderIndex < static_cast<int>(playOrder.size()))
			? playOrder[playOrderIndex] : -1;
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(playOrder.begin(), playOrder.end(), g);
		if (current >= 0) {
			auto it = std::find(playOrder.begin(), playOrder.end(), current);
			if (it != playOrder.end())
				std::iter_swap(it, playOrder.begin() + playOrderIndex);
		}
	}
	else {
		if (selectedIndex >= 0) playOrderIndex = selectedIndex;
	}
}

const int Playlist::getCurrentSongIndex() const {
	if (playOrderIndex < 0 || playOrderIndex >= static_cast<int>(playOrder.size())) return -1;
	return playOrder[playOrderIndex];
}