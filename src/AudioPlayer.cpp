#include "AudioPlayer.h"
#include <print>

AudioPlayer::AudioPlayer() {
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
	ma_sound_start(&mCurrentSound);
	return true;
}

bool AudioPlayer::stop() {
	if (!checkInit()) return false;

	if (mHasSound) {
		ma_sound_stop(&mCurrentSound);
		ma_sound_uninit(&mCurrentSound);
		mHasSound = false;
		return true;
	}
	return false;
}

bool AudioPlayer::pause() {
	if (!checkInit()) return false;

	if (mHasSound) {
		ma_sound_stop(&mCurrentSound);
		return true;
	}
	return false;
}

bool AudioPlayer::resume() {
	if (!checkInit()) return false;

	if (mHasSound) {
		ma_sound_start(&mCurrentSound);
		return true;
	}
	return false;
}

bool AudioPlayer::checkInit() {
	if (!mEngineInitialized) {
		std::println("Audio engine is not initialized.");
		return false;
	}
	return true;
}