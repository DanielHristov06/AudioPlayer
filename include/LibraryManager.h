#pragma once
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

struct Playlist {
	std::string name;
	fs::path filePath;
	std::vector<fs::path> songs;
	int selectedIndex = -1;
	std::vector<int> playOrder;
	int playOrderIndex = 0;
	bool shuffleEnabled = false;

	bool operator==(const Playlist& other) { return filePath == other.filePath; }

	void buildPlayOrder(bool shuffle);
	const int getCurrentSongIndex() const;
};

class LibraryManager {
public:
	LibraryManager();

	LibraryManager(const LibraryManager&) = delete;
	LibraryManager& operator=(const LibraryManager&) = delete;
	LibraryManager(LibraryManager&&) = delete;
	LibraryManager& operator=(LibraryManager&&) = delete;

	enum class PlayingMode { None, Queue, Playlist };
	PlayingMode playingMode = PlayingMode::None;

	std::vector<fs::path> mSongs;
	std::vector<Playlist> mPlaylists;
	std::vector<fs::path> mQueue;
	std::vector<int> mPlayOrder;

	int selectedPlaylist = -1;
	int selectedIndex = -1;
	int mPlayOrderIndex = 0;
	bool mShuffleEnabled = false;
	bool refreshing = false;

	bool import();
	bool erase(const fs::path& song);
	void buildPlayOrder(bool shuffle);
	const fs::path& getMainDir();
	const fs::path& getMusicDir();
	const int getCurrentSongIndex() const;

	void addSongToQueue(const fs::path& songPath);
	bool removeSongFromQueue(const fs::path& song);
	bool isSongInQueue(const fs::path path);

	bool createPlaylist(const std::string& playlist);
	bool removePlaylist(Playlist& playlist);
	void addSongToPlaylist(Playlist& playlist, const fs::path& songPath) const;
	bool removeSongFromPlaylist(Playlist& playlist, int songIndex) const;
	bool isSongInPlaylist(const fs::path& targetPath, const Playlist& playlist);
	void setPlaylistsShuffle(bool shuffle);

	void refreshSongs();

private:
	const char* mFilters[4] = { "*.mp3", "*.wav", "*.ogg", "*.flac" };
	fs::path mMainDir{};
	fs::path mMusicDir{};
	fs::path mPlaylistDir{};
};