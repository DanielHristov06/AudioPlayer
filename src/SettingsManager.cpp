#include "SettingsManager.h"
#include "Utils.h"
#include <string>
#include <print>
#include <fstream>

SettingsManager::SettingsManager() : mConfigPath(utils::getBasePath() / "AudioPlayer" / "Options.cfg") {}

bool SettingsManager::load(UIState& state) {
	std::ifstream file(mConfigPath, std::ios::app);

	if (!file.is_open()) {
		std::println("Failed to open Options.cfg: {}\n", mConfigPath.string());
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		const size_t pos = line.find(": ");
		if (pos == std::string::npos) continue;

		const std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 2);

		if (!value.empty() && value[0] == ' ') value.erase(0, 1);

		if (key == "Queue Background Color") {
			std::istringstream ss(value);
			ss >> state.queueBckColor[0] >> state.queueBckColor[1] >> state.queueBckColor[2] >> state.queueBckColor[3];
		}
		else if (key == "Player Background Color") {
			std::istringstream ss(value);
			ss >> state.playerColor[0] >> state.playerColor[1] >> state.playerColor[2] >> state.playerColor[3];
		}
		else if (key == "Songs Background Color") {
			std::istringstream ss(value);
			ss >> state.songsColor[0] >> state.songsColor[1] >> state.songsColor[2] >> state.songsColor[3];
		}
		else if (key == "Song list position") {
			std::istringstream ss(value);
			ss >> state.selectedSongsPos;
		}
	}

	file.close();
	return true;
}

bool SettingsManager::save(const UIState& state) {
	std::ofstream file(mConfigPath);

	if (!file.is_open()) {
		std::println("Failed to open Options.cfg: {}\n", mConfigPath.string());
		return false;
	}

	file << "Queue Background Color: " <<
		std::to_string(state.queueBckColor[0]) <<
		' ' << std::to_string(state.queueBckColor[1]) <<
		' ' << std::to_string(state.queueBckColor[2]) <<
		' ' << std::to_string(state.queueBckColor[3]) <<
		'\n';

	file << "Player Background Color: " <<
		std::to_string(state.playerColor[0]) <<
		' ' << std::to_string(state.playerColor[1]) <<
		' ' << std::to_string(state.playerColor[2]) <<
		' ' << std::to_string(state.playerColor[3]) <<
		'\n';

	file << "Songs Background Color: " <<
		std::to_string(state.songsColor[0]) <<
		' ' << std::to_string(state.songsColor[1]) <<
		' ' << std::to_string(state.songsColor[2]) <<
		' ' << std::to_string(state.songsColor[3]) <<
		'\n';

	file << "Song list position: " << std::to_string(state.selectedSongsPos);

	file.close();
	return true;
}