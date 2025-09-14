#include <string>
#include <iostream>
#include "AudioPlayer.h"

int main() {
	AudioPlayer player;
	std::string cmd;

	while (true) {
		std::getline(std::cin, cmd);

		switch (cmd[0]) {
		case 'q':
			return 0;
		case 'x':
			player.play("E:/Projects/AuidoPlayer/test_sounds/test.mp3");
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