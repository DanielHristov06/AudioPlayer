#pragma once
#include <string>
#include <filesystem>
#include "miniaudio.h"

class AudioPlayer {
public:
	AudioPlayer();
	~AudioPlayer() noexcept;

	AudioPlayer(const AudioPlayer&) = delete;
	AudioPlayer& operator=(const AudioPlayer&) = delete;
	AudioPlayer(AudioPlayer&&) = delete;
	AudioPlayer& operator=(AudioPlayer&&) = delete;

	bool play(const std::filesystem::path& path);
	bool stop() noexcept;
	bool pause() noexcept;
	bool resume() noexcept;
	bool isPaused() const noexcept;
	bool isPlaying() const noexcept;
	bool hasFinished() const noexcept;
	bool seek(double seconds) noexcept;
	bool forward(double seconds) noexcept;
	bool backward(double seconds) noexcept;

	double getCurrentTime() const noexcept;
	double getTotalTime() const noexcept;

	float getVolume() const noexcept;
	bool setVolume(float vol) noexcept;

private:
	bool checkInit() const;

	ma_engine mEngine{};
	ma_sound mCurrentSound{};
	bool mHasSound = false;
	bool mEngineInitialized = false;
	bool mPaused = false;
	float mVolume;
};