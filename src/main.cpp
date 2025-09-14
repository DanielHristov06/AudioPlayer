#include <string>
#include <iostream>
#include "AudioPlayer.h"
#include "tinyfiledialogs.h"

int main() {
	AudioPlayer player;
	std::string cmd;

	const char* filters[] = { "*.mp3", "*.wav", "*.ogg" };

	const char* filepath = tinyfd_openFileDialog("Select an audio file", "", 3, filters, "Audio files", 0);

	if (filepath) {
		while (true) {
			std::getline(std::cin, cmd);

			switch (cmd[0]) {
			case 'q':
				return 0;
			case 'x':
				player.play(filepath);
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
	}

	return 0;
}