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
		if (line.starts_with("Queue Background Color: ")) {
			const std::string values = line.substr(24);
			std::istringstream ss(values);
			ss >> state.queueBckColor[0] >> state.queueBckColor[1] >> state.queueBckColor[2] >> state.queueBckColor[3];
		}
		else if (line.starts_with("Player Background Color: ")) {
			const std::string values = line.substr(25);
			std::istringstream ss(values);
			ss >> state.playerColor[0] >> state.playerColor[1] >> state.playerColor[2] >> state.playerColor[3];
		}
		else if (line.starts_with("Songs Background Color: ")) {
			const std::string values = line.substr(25);
			std::istringstream ss(values);
			ss >> state.songsColor[0] >> state.songsColor[1] >> state.songsColor[2] >> state.songsColor[3];
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

	file.close();
	return true;
}