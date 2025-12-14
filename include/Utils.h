#pragma once
#include <string>
#include "LibraryManager.h"
#include "AudioPlayer.h"

std::string formatTime(double& seconds);
void playNextSong(LibraryManager& manager, AudioPlayer& player, int& selectedIndex);