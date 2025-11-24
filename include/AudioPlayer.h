#pragma once
#include <string>
#include "miniaudio.h"

class AudioPlayer {
public:
	AudioPlayer();
	~AudioPlayer();

	AudioPlayer(const AudioPlayer&) = delete;
	AudioPlayer& operator=(const AudioPlayer&) = delete;
	AudioPlayer(AudioPlayer&&) = delete;
	AudioPlayer& operator=(AudioPlayer&&) = delete;

	bool play(const std::string& path);
	bool stop();
	bool pause();
	bool resume();

private:
	bool checkInit();

	ma_engine mEngine{};
	ma_sound mCurrentSound{};
	bool mHasSound = false;
	bool mEngineInitialized = false;
};