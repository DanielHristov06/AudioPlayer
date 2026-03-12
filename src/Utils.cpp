#include "Utils.h"
#include <format>

namespace utils {
	std::string formatTime(const double& seconds) {
		const int s = static_cast<int>(seconds);
		const int min = s / 60;
		const int sec = s % 60;
		return std::format("{}:{:02}", min, sec);
	}

	void playNextSong(UIState& state, LibraryManager& manager, AudioPlayer& player) {
		if (manager.mSongs.empty()) return;

		if (!manager.isPlayingFomPlaylist) {
			if (manager.selectedIndex < 0) {
				manager.selectedIndex = 0;
			}
			else {
				manager.selectedIndex = (manager.selectedIndex + 1) % static_cast<int>(manager.mSongs.size());
			}

			player.play(manager.mSongs[manager.selectedIndex].string());
			state.currentlyPlayingPath = manager.mSongs[manager.selectedIndex];
		}
		else {
			Playlist& currPlaylist = manager.mPlaylists[manager.selectedPlaylist];

			if (currPlaylist.selectedIndex < 0) {
				currPlaylist.selectedIndex = 0;
			}
			else {
				currPlaylist.selectedIndex = (currPlaylist.selectedIndex + 1) % static_cast<int>(currPlaylist.songs.size());
			}

			player.play(currPlaylist.songs[currPlaylist.selectedIndex].string());
			state.currentlyPlayingPath = currPlaylist.songs[currPlaylist.selectedIndex];
		}
	}

	void playPrevSong(UIState& state, LibraryManager& manager, AudioPlayer& player){
		if (manager.mSongs.empty()) return;

		if (!manager.isPlayingFomPlaylist) {
			if (manager.selectedIndex < 0) {
				manager.selectedIndex = static_cast<int>(manager.mSongs.size() - 1);
			}
			else {
				const int size = static_cast<int>(manager.mSongs.size());
				manager.selectedIndex = (manager.selectedIndex - 1 + size) % size;
			}

			player.play(manager.mSongs[manager.selectedIndex].string());
			state.currentlyPlayingPath = manager.mSongs[manager.selectedIndex];
		}
		else {
			Playlist& currPlaylist = manager.mPlaylists[manager.selectedPlaylist];

			if (currPlaylist.selectedIndex < 0) {
				currPlaylist.selectedIndex = static_cast<int>(currPlaylist.songs.size() - 1);
			}
			else {
				const int size = static_cast<int>(currPlaylist.songs.size());
				currPlaylist.selectedIndex = (currPlaylist.selectedIndex - 1 + size) % size;
			}

			player.play(currPlaylist.songs[currPlaylist.selectedIndex].string());
			state.currentlyPlayingPath = currPlaylist.songs[currPlaylist.selectedIndex];
		}
	}
}