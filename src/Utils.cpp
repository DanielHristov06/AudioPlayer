#include "Utils.h"
#include <format>

namespace utils {
	std::string formatTime(double& seconds) {
		int s = static_cast<int>(seconds);
		int min = s / 60;
		int sec = s % 60;
		return std::format("{}:{:02}", min, sec);
	}

	void playNextSong(LibraryManager& manager, AudioPlayer& player) {
		if (manager.mSongs.empty()) return;

		manager.selectedIndex = (manager.selectedIndex + 1) % manager.mSongs.size();
		player.play(manager.mSongs[manager.selectedIndex].string());
	}

	void playPrevSong(LibraryManager& manager, AudioPlayer& player){
		if (manager.mSongs.empty()) return;

		manager.selectedIndex = (manager.selectedIndex - 1) % manager.mSongs.size();
		player.play(manager.mSongs[manager.selectedIndex].string());
	}
}