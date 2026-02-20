#pragma once
#include <string>
#include "LibraryManager.h"
#include "AudioPlayer.h"

namespace utils {
	std::string formatTime(double& seconds);
	void playNextSong(LibraryManager& manager, AudioPlayer& player);
	void playPrevSong(LibraryManager& manager, AudioPlayer& player);
}