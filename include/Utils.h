#pragma once
#include <string>
#include <glad/glad.h>
#include "LibraryManager.h"
#include "AudioPlayer.h"

namespace utils {
	struct UIState {
		GLuint playIcon{}, playIconHovered{};
		GLuint pauseIcon{}, pauseIconHovered{};
		GLuint volumeIcon{}, nextIcon{};

		// Song List State
		fs::path currentlyPlayingPath;
		int popupIndex = -1;
		int popupPlaylistIndex = -1;

		// Player State
		float volume = 1.0f;

		// Playlist Window
		bool playlistWindowOpen = false;
		char playlistName[128] = "";

		// Download WIndow
		bool downloadWindowOpen = false;
		char url[128] = "";
		int selectedFormat = 0;
		const char* formats[4] = { ".mp3", ".ogg", ".wav", ".flac" };
	};

	std::string formatTime(const double& seconds);
	void playNextSong(UIState& state, LibraryManager& manager, AudioPlayer& player);
	void playPrevSong(UIState& state, LibraryManager& manager, AudioPlayer& player);
	fs::path getBasePath();
	void createDirectory(const fs::path& dir);
	void openInExplorer(const fs::path& path);
}