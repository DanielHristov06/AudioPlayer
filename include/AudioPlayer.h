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
	bool isPaused() const;
	bool seek(double seconds);

	double getCurrentTime() const;
	double getTotalTime() const;

	float getVolume() const;
	bool setVolume(float vol);

private:
	bool checkInit() const;

	ma_engine mEngine{};
	ma_sound mCurrentSound{};
	bool mHasSound = false;
	bool mEngineInitialized = false;
	bool mPaused = false;
	float mVolume;
};