#include "AudioPlayer.h"
#include <print>

AudioPlayer::AudioPlayer() : mVolume(100.0f) {
	if (ma_engine_init(NULL, &mEngine) != MA_SUCCESS) {
		std::println("Failed to init audio engine.\n");
		mEngineInitialized = false;
		return;
	}

	mEngineInitialized = true;
}

AudioPlayer::~AudioPlayer() {
	if (!mEngineInitialized) return;

	if (mHasSound) {
		ma_sound_uninit(&mCurrentSound);
	}
	ma_engine_uninit(&mEngine);
}

bool AudioPlayer::play(const std::string& path) {
	if (!checkInit()) return false;

	if (mHasSound) {
		ma_sound_uninit(&mCurrentSound);
		mHasSound = false;
	}

	if (ma_sound_init_from_file(&mEngine, path.c_str(), NULL, NULL, NULL, &mCurrentSound) != MA_SUCCESS) {
		std::println("Failed to load sound from: {}\n", path);
		return false;
	}

	mHasSound = true;
	mPaused = false;
	ma_sound_start(&mCurrentSound);
	return true;
}

bool AudioPlayer::stop() {
	if (!checkInit()) return false;

	if (mHasSound) {
		ma_sound_stop(&mCurrentSound);
		ma_sound_uninit(&mCurrentSound);
		mHasSound = false;
		mPaused = false;
		return true;
	}
	return false;
}

bool AudioPlayer::pause() {
	if (!checkInit()) return false;

	if (mHasSound) {
		ma_sound_stop(&mCurrentSound);
		mPaused = true;
		return true;
	}
	return false;
}

bool AudioPlayer::resume() {
	if (!checkInit()) return false;

	if (mHasSound) {
		ma_sound_start(&mCurrentSound);
		mPaused = false;
		return true;
	}
	return false;
}

bool AudioPlayer::isPaused() const {
	if (!checkInit()) return false;
	return mPaused;
}

bool AudioPlayer::seek(double seconds) {
	if (!mHasSound || !checkInit()) return false;

	if (seconds < 0.0) seconds = 0.0;
	double total = getTotalTime();
	if (seconds > total) seconds = total;

	ma_result result = ma_sound_seek_to_second(&mCurrentSound, seconds);
	return result == MA_SUCCESS;
}

double AudioPlayer::getCurrentTime() const {
	if (!mHasSound || !checkInit()) return 0.0;

	float cursor = 0.0f;
	ma_sound_get_cursor_in_seconds(&mCurrentSound, &cursor);
	return double(cursor);
}

double AudioPlayer::getTotalTime() const {
	if (!mHasSound || !checkInit()) return 0.0;

	float length = 0.0f;
	ma_sound_get_length_in_seconds(&mCurrentSound, &length);
	return double(length);
}

float AudioPlayer::getVolume() const {
	if (!checkInit()) return 0.0f;
	return mVolume;
}

bool AudioPlayer::setVolume(float vol) {
	if (!mHasSound || !checkInit()) return false;

	ma_sound_set_volume(&mCurrentSound, vol);
	return true;
}

bool AudioPlayer::checkInit() const {
	if (!mEngineInitialized) {
		std::println("Audio engine is not initialized.");
		return false;
	}
	return true;
}