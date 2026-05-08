#include "AppUI.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Utils.h"
#include "Logger.h"

#include <string>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void AppUI::draw(UIState& state, SettingsManager& settings, LibraryManager& manager, AudioPlayer& player, Downloader& downloader, Downloader::DownloadStatus downloadStatus) {
	drawDockspace(state, manager, downloader);
	drawOtherWindow(state, manager, player);
	drawPlayer(state, manager, player);
	drawSongList(state, manager, player);
	drawPlaylistWindow(state, manager);
	drawDownloadWindow(state, manager, downloader, downloadStatus);
	drawOptionsWindow(state, settings);
	Logger::get().drawWindow();
}

void AppUI::drawDockspace(UIState& state, LibraryManager& manager, Downloader& downloader) {
	const ImGuiWindowFlags dockspaceFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::Begin("DockSpaceHost", nullptr, dockspaceFlags);
	ImGui::PopStyleVar(2);

	if (ImGui::BeginMenuBar()) {
		if (ImGui::MenuItem("Import")) {
			manager.import();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("New Playlist")) {
			state.playlistWindowOpen = !state.playlistWindowOpen;
			if (state.playlistWindowOpen) state.playlistName[0] = '\0';
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Download", nullptr, false, downloader.isReady())) {
			state.downloadWindowOpen = !state.downloadWindowOpen;
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Open in explorer")) {
			utils::openInExplorer(manager.getMainDir());
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Options")) {
			state.optionsWindowOpen = !state.optionsWindowOpen;
		}

		ImGui::EndMenuBar();
	}

	const ImGuiID dockspaceId = ImGui::GetID("Dockspace");

	if (state.firstTime || state.rebuildDock) {
		state.firstTime = false;
		state.rebuildDock = false;

		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

		ImGuiID dockMain = dockspaceId;
		ImGuiID dockBottom{}, dockSongs{};

		const ImGuiDir songsPos = state.selectedSongsPos == 0 ? ImGuiDir_Left : ImGuiDir_Right;

		ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.15f, &dockBottom, &dockMain);
		ImGui::DockBuilderSplitNode(dockMain, songsPos, 0.20f, &dockSongs, &dockMain);
		ImGui::DockBuilderGetNode(dockBottom)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
		ImGui::DockBuilderGetNode(dockSongs)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
		ImGui::DockBuilderGetNode(dockMain)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
		ImGui::DockBuilderDockWindow("Player", dockBottom);
		ImGui::DockBuilderDockWindow("Songs", dockSongs);
		ImGui::DockBuilderDockWindow("Other", dockMain);
		ImGui::DockBuilderFinish(dockspaceId);
	}

	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));

	ImGui::End();
}

void AppUI::drawOtherWindow(UIState& state, LibraryManager& manager, AudioPlayer& player) {
	ImGui::PushStyleColor(ImGuiCol_WindowBg,
		ImVec4(state.queueBckColor[0], state.queueBckColor[1], state.queueBckColor[2], state.queueBckColor[3]));

	ImGui::Begin("Other");

	const bool bothVisible = state.queueEnabled && state.historyEnabled;
	const float otherSpacing = ImGui::GetStyle().ItemSpacing.x;
	const float availWidth = ImGui::GetContentRegionAvail().x;
	const float childWidth = bothVisible ? (availWidth * 0.5f - otherSpacing * 0.5f) : availWidth;
	const float childHeight = ImGui::GetContentRegionAvail().y;

	if (state.queueEnabled) {
		ImGui::BeginChild("##QueueChild", ImVec2(childWidth, childHeight), true);

		ImGui::Text("Queue:");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Clear queue").x - 10.0f + ImGui::GetCursorPosX());

		if (ImGui::Button("Clear Queue")) {
			manager.mQueue.clear();
			manager.playingMode = LibraryManager::PlayingMode::None;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (!manager.mQueue.empty()) {
			for (size_t i = 0; i < manager.mQueue.size(); i++) {
				const auto& song = manager.mQueue[i];
				utils::renderSongSelectable(song, LibraryManager::PlayingMode::Queue, i, -1, "##queue_" + std::to_string(i), "QueueSongContextMenu", state, manager, player);
			}

			if (ImGui::BeginPopup("QueueSongContextMenu")) {
				const fs::path& popupSong = manager.mQueue[state.popupIndex];

				if (ImGui::MenuItem("Play")) {
					player.play(popupSong);
					manager.addSongToHistory(popupSong);
					state.currentlyPlayingPath = popupSong;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Remove from queue")) {
					manager.removeSongFromQueue(popupSong);
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Add to playlist")) {
					for (auto& playlist : manager.mPlaylists) {
						const bool alreadyIn = manager.isSongInPlaylist(popupSong, playlist);
						if (ImGui::MenuItem(playlist.name.c_str(), nullptr, false, !alreadyIn)) {
							manager.addSongToPlaylist(playlist, popupSong);
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::EndChild();
	}

	if (state.historyEnabled) {
		if (bothVisible) ImGui::SameLine();

		ImGui::BeginChild("##HistoryChild", ImVec2(childWidth, childHeight), true);

		ImGui::Text("History:");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Clear History").x - 10.0f + ImGui::GetCursorPosX());

		if (ImGui::Button("Clear History")) {
			manager.mHistory.clear();
			manager.playingMode = LibraryManager::PlayingMode::None;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (!manager.mHistory.empty()) {
			for (size_t i = 0; i < manager.mHistory.size(); i++) {
				const auto& song = manager.mHistory[i];
				utils::renderSongSelectable(song, LibraryManager::PlayingMode::History, i, -1, "##ll_" + std::to_string(i), "HistorySongContextMenu", state, manager, player);
			}

			if (ImGui::BeginPopup("HistorySongContextMenu")) {
				const fs::path& popupSong = manager.mHistory[state.popupIndex];

				if (ImGui::MenuItem("Play")) {
					player.play(popupSong);
					manager.addSongToHistory(popupSong);
					state.currentlyPlayingPath = popupSong;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Remove from History")) {
					manager.removeSongFromHistory(popupSong);
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Add to playlist")) {
					for (auto& playlist : manager.mPlaylists) {
						const bool alreadyIn = manager.isSongInPlaylist(popupSong, playlist);
						if (ImGui::MenuItem(playlist.name.c_str(), nullptr, false, !alreadyIn)) {
							manager.addSongToPlaylist(playlist, popupSong);
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::EndChild();
	}

	ImGui::End();
	ImGui::PopStyleColor();
}

void AppUI::drawPlayer(UIState& state, LibraryManager& manager, AudioPlayer& player) {
	ImGui::PushStyleColor(ImGuiCol_WindowBg,
		ImVec4(state.playerColor[0], state.playerColor[1], state.playerColor[2], state.playerColor[3]));

	ImGui::Begin("Player");
	const float curPos = ImGui::GetCursorPosX();

	// Song Name
	if (manager.selectedIndex >= 0 || manager.selectedPlaylist >= 0) {
		switch (manager.playingMode)
		{
		case LibraryManager::PlayingMode::None: {
			ImGui::Text("%s", utils::toUtf8(manager.mSongs[manager.selectedIndex].stem()).c_str());
			break;
		}
		case LibraryManager::PlayingMode::Queue: {
			ImGui::Text("%s", utils::toUtf8(manager.mQueue[manager.selectedIndex].stem()).c_str());
			break;
		}
		case LibraryManager::PlayingMode::History: {
			ImGui::Text("%s", utils::toUtf8(manager.mHistory[manager.selectedIndex].stem()).c_str());
			break;
		}
		case LibraryManager::PlayingMode::Playlist: {
			const Playlist& selectedPlaylist = manager.mPlaylists[manager.selectedPlaylist];
			ImGui::Text("%s", utils::toUtf8(selectedPlaylist.songs[selectedPlaylist.selectedIndex].stem()).c_str());
			break;
		}
		}

		ImGui::SameLine();
		ImGui::SetCursorPosX(curPos);
	}

	const ImVec4 transparent(0, 0, 0, 0);

	ImGui::PushStyleColor(ImGuiCol_Button, transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);

	const float playSize = std::clamp(ImGui::GetWindowSize().x * 0.04f, 32.0f, 48.0f);
	const float bigSize = 64.0f;
	const float smallSize = 24.0f;
	const float playerSpacing = 12.0f;

	const float rowHeight = bigSize;

	const float totalWidth =
		smallSize +
		playerSpacing + smallSize +
		playerSpacing + bigSize +
		playerSpacing + playSize +
		playerSpacing + bigSize +
		playerSpacing + bigSize +
		playerSpacing + smallSize;

	const float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

	const ImVec2 rowStart = ImGui::GetCursorPos();

	auto CenteredIconButton = [&](const char* id, ImTextureID texture, float size, ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1)) {
		const float yOffset = (rowHeight - size) * 0.5f;
		ImGui::SetCursorPosY(rowStart.y + yOffset - 10.0f);
		return ImGui::ImageButton(id, ImTextureRef(texture), ImVec2(size, size), uv0, uv1);
		};

	// Shuffle Button
	if (CenteredIconButton("Shuffle", (ImTextureID)state.shuffleIcon, smallSize)) {
		manager.mShuffleEnabled = !manager.mShuffleEnabled;
		manager.buildPlayOrder(manager.mShuffleEnabled);
		manager.setPlaylistsShuffle(manager.mShuffleEnabled);
	}

	ImGui::SameLine(0.0f, playerSpacing);

	// Backward Button
	if (CenteredIconButton("Backward", (ImTextureID)state.forwardIcon, smallSize, ImVec2(1, 0), ImVec2(0, 1))) {
		player.backward(state.backwardSkip);
	}

	ImGui::SameLine(0.0f, playerSpacing);

	// Previous Button
	if (CenteredIconButton("Previous", (ImTextureID)state.nextIcon, bigSize, ImVec2(1, 0), ImVec2(0, 1))) {
		utils::playPrevSong(state, manager, player);
	}

	ImGui::SameLine(0.0f, playerSpacing);

	// Play Button
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const ImVec2 sizeVec(playSize, playSize);
	const ImRect rect(pos, ImVec2(pos.x + sizeVec.x, pos.y + sizeVec.y));
	bool hovered = rect.Contains(ImGui::GetIO().MousePos);
	GLuint tex{};
	const bool paused = player.isPaused();
	if (!paused) {
		tex = hovered ? state.pauseIconHovered : state.pauseIcon;
	}
	else {
		tex = hovered ? state.playIconHovered : state.playIcon;
	}

	if (CenteredIconButton("PlayButton", tex, playSize)) {
		paused ? player.resume() : player.pause();
	}

	ImGui::SameLine(0.0f, playerSpacing);

	// Next Button
	if (CenteredIconButton("Next", (ImTextureID)state.nextIcon, bigSize)) {
		utils::playNextSong(state, manager, player);
	}

	ImGui::SameLine(0.0f, playerSpacing);

	// Forward Button
	if (CenteredIconButton("Forward", (ImTextureID)state.forwardIcon, smallSize)) {
		player.forward(state.forwardSkip);
	}

	ImGui::SameLine(0.0f, playerSpacing);

	// Repeat Button
	if (CenteredIconButton("RepeatButton", (ImTextureID)state.repeatIcon, smallSize)) {
		switch (state.repeatState) {
		case UIState::RepeatState::Off:
			state.repeatState = UIState::RepeatState::Once;
			break;

		case UIState::RepeatState::Once:
			state.repeatState = UIState::RepeatState::Always;
			break;

		case UIState::RepeatState::Always:
			state.repeatState = UIState::RepeatState::Off;
			break;
		}
	}

	ImGui::SetCursorPosY(rowStart.y + rowHeight);

	ImGui::PopStyleColor(3);

	// Progress Bar
	double current = player.getCurrentTime();
	double total = player.getTotalTime();
	float progress = (total > 0.0f) ? float(current / total) : 0.0f;
	if (state.isSeeking) progress = state.seekPreview;
	const double displayTime = state.isSeeking ? (state.seekPreview * total) : current;
	const std::string leftTime = utils::formatTime(displayTime);
	const std::string rightTime = utils::formatTime(total);

	const float textWidth = ImGui::CalcTextSize("0:00").x;
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float barWidth = ImGui::GetContentRegionAvail().x * 0.3f;
	const float totalRowWidth = textWidth + spacing + barWidth + spacing + textWidth;

	const float offset = (ImGui::GetContentRegionAvail().x - totalRowWidth) * 0.5f;
	if (offset > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

	ImGui::Text("%s", leftTime.c_str());
	ImGui::SameLine();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, -1.0f));
	ImGui::PushItemWidth(barWidth);
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

	ImGui::SliderFloat("##SongBar", &progress, 0.0f, 1.0f, "");

	if (ImGui::IsItemActive()) {
		state.isSeeking = true;
		state.seekPreview = progress;
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		player.seek(state.seekPreview * total);
		state.isSeeking = false;
	}

	ImGui::PopStyleVar();
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::Text("%s", rightTime.c_str());
	ImGui::SameLine();

	// Volume Bar
	const float x = ImGui::GetCursorPosX();
	ImGui::SetCursorPosX(x + ImGui::GetContentRegionAvail().x - 200.0f);
	ImGui::Image(ImTextureRef((ImTextureID)state.volumeIcon), ImVec2(16.0f, 16.0f));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.0f);
	const float y = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY(y + 2.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, -1.0f));
	if (ImGui::SliderFloat("##VolumeBar", &state.volume, 0.0f, 1.0f, "")) {
		player.setVolume(state.volume);
	}
	ImGui::PopStyleVar(1);
	ImGui::SameLine();

	ImGui::Text("%.0f", state.volume * 100.0f);

	ImGui::End();
	ImGui::PopStyleColor();
}

void AppUI::drawSongList(UIState& state, LibraryManager& manager, AudioPlayer& player) {
	const ImVec4 transparent(0, 0, 0, 0);

	ImGui::PushStyleColor(ImGuiCol_WindowBg,
		ImVec4(state.songsColor[0], state.songsColor[1], state.songsColor[2], state.songsColor[3]));

	// Song List
	ImGui::Begin("Songs");
	ImGui::BeginChild("SongList", ImVec2(0, 0), true);

	if (!state.searchOpen) {
		ImGui::Text("Song List:");
	}
	else {
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
		ImGui::InputText("##SearchBar", state.searchQuery, IM_ARRAYSIZE(state.searchQuery));
	}

	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f + ImGui::GetCursorPosX());

	ImGui::PushStyleColor(ImGuiCol_Button, transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);

	if (ImGui::ImageButton("Search", ImTextureRef((ImTextureID)state.searchIcon), ImVec2(18.0f, 18.0f))) {
		state.searchOpen = !state.searchOpen;
		if (!state.searchOpen) state.searchQuery[0] = '\0';
	}

	ImGui::SameLine();

	if (ImGui::ImageButton("Refresh", ImTextureRef((ImTextureID)state.refreshIcon), ImVec2(18.0f, 18.0f))) {
		if (!manager.refreshing) manager.refreshSongs();
	}

	ImGui::PopStyleColor(3);

	ImGui::Dummy(ImVec2(0.0f, 4.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	const std::string query = utils::toLower(state.searchQuery);
	const bool isSearching = !query.empty();

	if (isSearching) {
		if (ImGui::CollapsingHeader("All Songs")) {
			for (size_t i = 0; i < manager.mSongs.size(); i++) {
				const auto& song = manager.mSongs[i];
				const std::string stem = utils::toLower(utils::toUtf8(song.stem()));
				if (stem.find(query) != std::string::npos) {
					utils::renderSongSelectable(song, LibraryManager::PlayingMode::None, i, -1, "##main_" + std::to_string(i), "SongContextMenu", state, manager, player);
				}
			}
		}
	}
	else {
		if (ImGui::CollapsingHeader("All Songs")) {
			for (size_t i = 0; i < manager.mSongs.size(); i++) {
				const auto& song = manager.mSongs[i];
				utils::renderSongSelectable(song, LibraryManager::PlayingMode::None, i, -1, "##main_" + std::to_string(i), "SongContextMenu", state, manager, player);
			}
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (isSearching) {
		for (size_t p = 0; p < manager.mPlaylists.size(); p++) {
			auto& playlist = manager.mPlaylists[p];
			if (utils::toLower(playlist.name).find(query) != std::string::npos) {
				const bool headerOpen = ImGui::CollapsingHeader(playlist.name.c_str());
				const bool headerHovered = ImGui::IsItemHovered();

				if (headerOpen) {
					for (size_t i = 0; i < playlist.songs.size(); i++) {
						const auto& song = playlist.songs[i];
						utils::renderSongSelectable(song, LibraryManager::PlayingMode::Playlist, i, p, "##pl_" + std::to_string(p) + "_" + std::to_string(i), "SongContextMenu", state, manager, player);
					}

					if (headerHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
						ImGui::OpenPopup("PlaylistContextMenu");
						state.popupPlaylistIndex = p;
					}
				}
			}
		}
	}
	else {
		for (size_t p = 0; p < manager.mPlaylists.size(); p++) {
			auto& playlist = manager.mPlaylists[p];
			const bool headerOpen = ImGui::CollapsingHeader(playlist.name.c_str());
			const bool headerHovered = ImGui::IsItemHovered();

			if (headerOpen) {
				for (size_t i = 0; i < playlist.songs.size(); i++) {
					const auto& song = playlist.songs[i];
					utils::renderSongSelectable(song, LibraryManager::PlayingMode::Playlist, i, p, "##pl_" + std::to_string(p) + "_" + std::to_string(i), "SongContextMenu", state, manager, player);
				}

				if (headerHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup("PlaylistContextMenu");
					state.popupPlaylistIndex = p;
				}
			}
		}
	}

	if (ImGui::BeginPopup("SongContextMenu")) {
		const fs::path& popupSong = (state.popupPlaylistIndex == -1)
			? manager.mSongs[state.popupIndex] : manager.mPlaylists[state.popupPlaylistIndex].songs[state.popupIndex];

		if (ImGui::MenuItem("Play")) {
			player.play(popupSong);
			manager.addSongToHistory(popupSong);
			state.currentlyPlayingPath = popupSong;
			ImGui::CloseCurrentPopup();
		}

		if (state.popupPlaylistIndex == -1) {
			if (ImGui::MenuItem("Delete")) {
				manager.erase(manager.mSongs[state.popupIndex]);
				ImGui::CloseCurrentPopup();
			}
		}
		else {
			if (ImGui::MenuItem("Remove from playlist")) {
				manager.removeSongFromPlaylist(manager.mPlaylists[state.popupPlaylistIndex], state.popupIndex);
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Add to queue")) {
			manager.addSongToQueue(popupSong);
		}

		if (ImGui::BeginMenu("Add to playlist")) {
			for (auto& playlist : manager.mPlaylists) {
				const bool alreadyIn = manager.isSongInPlaylist(popupSong, playlist);
				if (ImGui::MenuItem(playlist.name.c_str(), nullptr, false, !alreadyIn)) {
					manager.addSongToPlaylist(playlist, popupSong);
				}
			}

			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("PlaylistContextMenu")) {
		if (ImGui::MenuItem("Delete playlist")) {
			manager.removePlaylist(manager.mPlaylists[state.popupPlaylistIndex]);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::EndChild();
	ImGui::End();

	ImGui::PopStyleColor();
}

void AppUI::drawPlaylistWindow(UIState& state, LibraryManager& manager) {
	if (state.playlistWindowOpen) {
		ImGui::SetNextWindowSize(ImVec2(350, 180));

		ImGui::Begin("New Playlist", &state.playlistWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		const float windowWidth = ImGui::GetWindowSize().x;
		const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);

		const char* labelText = "Playlist Name";
		const float labelWidth = ImGui::CalcTextSize(labelText).x;
		ImGui::SetCursorPosX((windowWidth - labelWidth) * 0.5f);
		ImGui::Text("%s", labelText);
		ImGui::Dummy(widgetSpacing);

		const float inputWidth = windowWidth * 0.8f;
		ImGui::SetCursorPosX((windowWidth - inputWidth) * 0.5f);
		ImGui::PushItemWidth(inputWidth);
		ImGui::InputText("##NameInput", state.playlistName, IM_ARRAYSIZE(state.playlistName));
		ImGui::PopItemWidth();

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const float buttonWidth = 50.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalButtonWidth = (buttonWidth * 2) + spacing;
		ImGui::SetCursorPosX((windowWidth - totalButtonWidth) * 0.5f);
		if (ImGui::Button("Cancel")) {
			state.playlistWindowOpen = false;
			state.playlistName[0] = '\0';
		}
		ImGui::SameLine();

		if (ImGui::Button("Create")) {
			const std::string plName(state.playlistName);
			manager.createPlaylist(plName);
			state.playlistName[0] = '\0';
		}

		ImGui::End();
	}
}

void AppUI::drawDownloadWindow(UIState& state, LibraryManager& manager, Downloader& downloader, Downloader::DownloadStatus downloadStatus) {
	if (state.downloadWindowOpen) {
		ImGui::SetNextWindowSize(ImVec2(350, 180));

		ImGui::Begin("Download a song", &state.downloadWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		const float windowWidth = ImGui::GetWindowSize().x;
		const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);

		const char* labelText = "URL";
		const float labelWidth = ImGui::CalcTextSize(labelText).x;
		ImGui::SetCursorPosX((windowWidth - labelWidth) * 0.5f);
		ImGui::Text("%s", labelText);
		ImGui::Dummy(widgetSpacing);

		const float inputWidth = windowWidth * 0.75f;
		ImGui::PushItemWidth(inputWidth);
		ImGui::InputText("##UrlInput", state.url, IM_ARRAYSIZE(state.url));
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::SetNextItemWidth(65.0f);
		ImGui::Combo("##Format", &state.selectedFormat, state.formats, IM_ARRAYSIZE(state.formats));

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const float buttonWidth = 50.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalButtonWidth = (buttonWidth * 2) + spacing;
		ImGui::SetCursorPosX((windowWidth - totalButtonWidth) * 0.5f);
		if (ImGui::Button("Cancel")) {
			state.downloadWindowOpen = false;
			state.url[0] = '\0';
		}
		ImGui::SameLine();

		if (ImGui::Button("Download")) {
			std::string format = std::string(state.formats[state.selectedFormat]).substr(1);
			downloader.download(state.url, manager.getMusicDir(), format);
			state.url[0] = '\0';
		}

		std::string statusText = "";

		switch (downloadStatus) {
		case Downloader::DownloadStatus::Downloading:
			statusText = "Downloading..."; break;
		case Downloader::DownloadStatus::Failed:
			statusText = "Download Failed"; break;
		case Downloader::DownloadStatus::Success:
			statusText = "Download Successful"; break;
		}

		ImGui::Text("%s", statusText.c_str());

		ImGui::End();
	}
}

void AppUI::drawOptionsWindow(UIState& state, SettingsManager& settings) {
	if (state.optionsWindowOpen) {
		ImGui::Begin("Options", &state.optionsWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse);
		const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);
		const float windowWidth = ImGui::GetWindowSize().x;

		ImGui::ColorEdit4("Queue Background Color", state.queueBckColor);
		if (ImGui::Button("Reset to default##_queueBck")) {
			std::copy(std::begin(state.queueDefaultBckColor), std::end(state.queueDefaultBckColor), state.queueBckColor);
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		ImGui::ColorEdit4("Player Background Color", state.playerColor);
		if (ImGui::Button("Reset to default##_mPlayerBck")) {
			std::copy(std::begin(state.playerDefaultColor), std::end(state.playerDefaultColor), state.playerColor);
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		ImGui::ColorEdit4("Songs Background Color", state.songsColor);
		if (ImGui::Button("Reset to default##_songsBck")) {
			std::copy(std::begin(state.songsDefaultColor), std::end(state.songsDefaultColor), state.songsColor);
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		if (ImGui::Combo("Song list position", &state.selectedSongsPos, state.songsPos, IM_ARRAYSIZE(state.songsPos))) {
			state.rebuildDock = true;
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		ImGui::Checkbox("Enable queue", &state.queueEnabled);
		ImGui::SameLine();
		ImGui::Checkbox("Enable history", &state.historyEnabled);

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const double min = 0.1, max = 30.0;
		ImGui::DragScalar("Forward skip seconds", ImGuiDataType_Double, &state.forwardSkip, 0.1, &min, &max, "%.2f");
		ImGui::DragScalar("Backward skip seconds", ImGuiDataType_Double, &state.backwardSkip, 0.1, &min, &max, "%.2f");

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const float buttonWidth = (ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Apply").x) / 2.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalButtonWidth = (buttonWidth * 2) + spacing;
		ImGui::SetCursorPosX((windowWidth - totalButtonWidth) * 0.5f);
		if (ImGui::Button("Cancel")) {
			state.optionsWindowOpen = false;
		}
		ImGui::SameLine();

		if (ImGui::Button("Apply")) {
			settings.save(state);
		}

		ImGui::End();
	}
}