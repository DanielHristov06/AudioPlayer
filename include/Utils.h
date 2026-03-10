#pragma once
#include <string>
#include "LibraryManager.h"
#include "AudioPlayer.h"

namespace utils {
	std::string formatTime(const double& seconds);
	void playNextSong(LibraryManager& manager, AudioPlayer& player);
	void playPrevSong(LibraryManager& manager, AudioPlayer& player);
}