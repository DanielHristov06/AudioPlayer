#pragma once

#include "UIState.h"
#include "SettingsManager.h"
#include "LibraryManager.h"
#include "AudioPlayer.h"
#include "Downloader.h"

class AppUI {
public:
	AppUI() = default;

	AppUI(const AppUI&) = delete;
	AppUI& operator=(const AppUI&) = delete;
	AppUI(AppUI&&) = delete;
	AppUI& operator=(AppUI&&) = delete;

	void draw(UIState& state, SettingsManager& settings, LibraryManager& manager, AudioPlayer& player, Downloader& downloader, Downloader::DownloadStatus downloadStatus);

private:
	void drawDockspace(UIState& state, LibraryManager& manager, Downloader& downloader);
	void drawOtherWindow(UIState& state, LibraryManager& manager, AudioPlayer& player);
	void drawPlayer(UIState& state, LibraryManager& manager, AudioPlayer& player);
	void drawSongList(UIState& state, LibraryManager& manager, AudioPlayer& player);
	void drawPlaylistWindow(UIState& state, LibraryManager& manager);
	void drawDownloadWindow(UIState& state, LibraryManager& manager, Downloader& downloader, Downloader::DownloadStatus downloadStatus);
	void drawOptionsWindow(UIState& state, SettingsManager& settings);
};