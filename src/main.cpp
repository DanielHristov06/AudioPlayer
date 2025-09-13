#include <print>
#include <iostream>
#include <miniaudio.h>

int main() {
	ma_engine engine;
	if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
		std::println("Failed to init audio engine.\n");
		return -1;
	}

	const char* filepath = "E:/Projects/AuidoPlayer/test_sounds/test.mp3";

	if (ma_engine_play_sound(&engine, filepath, NULL) != MA_SUCCESS) {
		std::println("Failed to play sound: {}\n", filepath);
		ma_engine_uninit(&engine);
		return -1;
	}

	std::cin.get();

	ma_engine_uninit(&engine);
	return 0;
}