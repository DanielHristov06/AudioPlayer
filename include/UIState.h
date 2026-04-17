#pragma once
#include <glad/glad.h>
#include <filesystem>

struct UIState {
	GLuint playIcon{}, playIconHovered{};
	GLuint pauseIcon{}, pauseIconHovered{};
	GLuint volumeIcon{}, nextIcon{}, repeatIcon{};
	GLuint shuffleIcon{}, searchIcon{}, refreshIcon{};

	// Repeat State
	enum class RepeatState { Off, Once, Always };
	RepeatState repeatState = RepeatState::Off;
	bool repeatUsed = false;

	// Song List State
	std::filesystem::path currentlyPlayingPath{};
	int popupIndex = -1;
	int popupPlaylistIndex = -1;

	// Player State
	float volume = 1.0f;
	float seekPreview = 0.0f;
	bool isSeeking = false;

	// Playlist Window
	bool playlistWindowOpen = false;
	char playlistName[128] = "";

	// Download Window
	bool downloadWindowOpen = false;
	char url[128] = "";
	int selectedFormat = 0;
	const char* formats[4] = { ".mp3", ".ogg", ".wav", ".flac" };

	// Search
	bool searchOpen = false;
	char searchQuery[128] = "";

	// Options Window
	bool optionsWindowOpen = false;
	float queueDefaultBckColor[4] = { 0.08f, 0.08f, 0.08f, 1.0f };
	float queueBckColor[4] = {0.08f, 0.08f, 0.08f, 1.0f};
	
	float playerDefaultColor[4] = { 0.0588f, 0.0588f, 0.0588f, 1.0f };
	float playerColor[4] = { 0.0588f, 0.0588f, 0.0588f, 1.0f };

	float songsDefaultColor[4] = { 0.0588f, 0.0588f, 0.0588f, 1.0f };
	float songsColor[4] = { 0.0588f, 0.0588f, 0.0588f, 1.0f };
};