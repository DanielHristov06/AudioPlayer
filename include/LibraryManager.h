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

	bool operator==(const Playlist& other) { return filePath == other.filePath; }
};

class LibraryManager {
public:
	LibraryManager();

	std::vector<fs::path> mSongs;
	std::vector<Playlist> mPlaylists;
	int selectedPlaylist;
	int selectedIndex;
	bool isPlayingFomPlaylist;
	bool refreshing;
	bool import();
	bool erase(const fs::path& song);
	const fs::path& getMusicDir();

	bool createPlaylist(const std::string& playlist);
	bool removePlaylist(Playlist& playlist);
	void addSongToPlaylist(Playlist& playlist, const fs::path& songPath) const;
	bool removeSongFromPlaylist(Playlist& playlist, int songIndex) const;
	bool isSongInPlaylist(const fs::path& targetPath, const Playlist& playlist);

	void refreshSongs();

private:
	const char* mFilters[3] = { "*.mp3", "*.wav", "*.ogg" };
	fs::path mMainDir{};
	fs::path mMusicDir{};
	fs::path mPlaylistDir{};
};