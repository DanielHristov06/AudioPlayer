#include <iostream>
#include <print>
#include "AudioPlayer.h"
#include "LibraryManager.h"
#include "tinyfiledialogs.h"

int main() {
	LibraryManager manager;
	manager.import();

	AudioPlayer player;
	std::string cmd;
	std::string song;

	std::getline(std::cin, song);
	auto& filepath = manager.mSongs[std::stoi(song) - 1];

	while (true) {
		int i = 0;
		for (const auto& entry : manager.mSongs) {
			std::println("{}: {}", i + 1, entry.stem().string());
			i++;
		}

		std::getline(std::cin, cmd);

		switch (cmd[0]) {
			case 'q':
				return 0;
			case 'x':
				player.play(filepath.string());
				break;
			case 's':
				player.stop();
				break;
			case 'p':
				player.pause();
				break;
			case 'r':
				player.resume();
				break;
			default:
				std::cout << "Unkown command\n";
				break;
		}
	}

	return 0;
}