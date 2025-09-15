#pragma once
#include <string>
#include "miniaudio.h"

class AudioPlayer {
public:
	AudioPlayer();
	~AudioPlayer();

	bool play(const std::string& path);
	bool stop();
	bool pause();
	bool resume();

private:
	ma_engine mEngine;
	ma_sound mCurrentSound;
	bool mHasSound = false;
};