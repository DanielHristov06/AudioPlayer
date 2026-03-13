#pragma once
#include <filesystem>
#include <vector>
#include <string>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(dlp);

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
	bool import();
	bool erase(const fs::path& song);

	bool createPlaylist(const std::string& playlist);
	bool removePlaylist(Playlist& playlist);
	void addSongToPlaylist(Playlist& playlist, const fs::path& songPath) const;
	bool removeSongFromPlaylist(Playlist& playlist, int songIndex) const;
	bool isSongInPlaylist(const fs::path& targetPath, const Playlist& playlist);

private:
	const char* mFilters[3] = { "*.mp3", "*.wav", "*.ogg" };
	fs::path mMainDir{};
	fs::path mMusicDir{};
	fs::path mPlaylistDir{};
	fs::path mYtDlpDir{};
	fs::path mYtDlpPath{};
	fs::path getBasePath();
	bool extractYtDlp();
	void createDirectory(const fs::path& dir);
};