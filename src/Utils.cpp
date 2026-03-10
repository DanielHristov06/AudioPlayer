#include "Utils.h"
#include <format>

namespace utils {
	std::string formatTime(const double& seconds) {
		const int s = static_cast<int>(seconds);
		const int min = s / 60;
		const int sec = s % 60;
		return std::format("{}:{:02}", min, sec);
	}

	void playNextSong(LibraryManager& manager, AudioPlayer& player) {
		if (manager.mSongs.empty()) return;

		if (manager.selectedIndex < 0) {
			manager.selectedIndex = 0;
		}
		else {
			manager.selectedIndex = (manager.selectedIndex + 1) % manager.mSongs.size();
		}
		
		player.play(manager.mSongs[manager.selectedIndex].string());
	}

	void playPrevSong(LibraryManager& manager, AudioPlayer& player){
		if (manager.mSongs.empty()) return;

		if (manager.selectedIndex < 0) {
			manager.selectedIndex = static_cast<int>(manager.mSongs.size() - 1);
		}
		else {
			const int size = static_cast<int>(manager.mSongs.size());
			manager.selectedIndex = (manager.selectedIndex - 1 + size) % size;
		}

		player.play(manager.mSongs[manager.selectedIndex].string());
	}
}