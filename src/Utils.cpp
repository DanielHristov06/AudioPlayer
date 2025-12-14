#include "Utils.h"
#include <format>

std::string formatTime(double& seconds) {
	int s = static_cast<int>(seconds);
	int min = s / 60;
	int sec = s % 60;
	return std::format("{}:{:02}", min, sec);
}

void playNextSong(LibraryManager& manager, AudioPlayer& player, int& selectedIndex) {
	if (manager.mSongs.empty()) return;

	selectedIndex = (selectedIndex + 1) % manager.mSongs.size();
	player.play(manager.mSongs[selectedIndex].string());
}