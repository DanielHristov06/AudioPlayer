#pragma once
#include <string>
#include "AudioPlayer.h"
#include "LibraryManager.h"
#include "UIState.h"

namespace utils {
	std::string formatTime(const double& seconds);
	void playNextSong(UIState& state, LibraryManager& manager, AudioPlayer& player);
	void playPrevSong(UIState& state, LibraryManager& manager, AudioPlayer& player);
	fs::path getBasePath();
	void createDirectory(const fs::path& dir);
	void openInExplorer(const fs::path& path);

	void renderSongSelectable(const fs::path& song, LibraryManager::PlayingMode playingMode, int index, int playlistIndex,
		const std::string& labelSuffix, const char* popupName, UIState& state, LibraryManager& manager, AudioPlayer& player);

	std::string toUtf8(const fs::path& path);
}