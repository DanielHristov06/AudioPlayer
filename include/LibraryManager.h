#pragma once
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

struct Playlist {
	std::string name;
	fs::path filePath;
	std::vector<fs::path> songs;
	int selectedIndex = -1;
};

class LibraryManager {
public:
	LibraryManager();

	std::vector<fs::path> mSongs;
	std::vector<Playlist> mPlaylists;
	int selectedPlaylist;
	int selectedIndex;
	bool isPlayingFomPlaylist;
	bool import();
	bool erase(const fs::path& song);

	bool createPlaylist(const std::string& playlist);
	void addSongToPlaylist(Playlist& playlist, const fs::path& songPath) const;
	bool removeSongFromPlaylist(Playlist& playlist, int songIndex) const;
	bool isSongInPlaylist(const fs::path& targetPath, const Playlist& playlist);

private:
	const char* mFilters[3] = { "*.mp3", "*.wav", "*.ogg" };
	fs::path mMainDir{};
	fs::path mMusicDir{};
	fs::path mPlaylistDir{};
	fs::path getBasePath();
	void createDirectory(const fs::path& dir);
};