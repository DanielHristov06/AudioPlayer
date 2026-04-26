#include <print>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "tinyfiledialogs.h"
#include "LibraryManager.h"
#include "AudioPlayer.h"
#include "UIState.h"
#include "SettingsManager.h"
#include "Downloader.h"
#include "TextureLoader.h"
#include "Utils.h"

int main() {
	UIState state;
	SettingsManager settings;
	settings.load(state);

	LibraryManager manager;
	Downloader downloader;
	AudioPlayer player;

	if (!glfwInit()) {
		std::println("Could not initialize GLFW.\n");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Audio Player", NULL, NULL);
	if (!window) {
		std::println("Failed to create a window.\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		glViewport(0, 0, width, height);
	});

	glfwSetWindowUserPointer(window, &manager);
	glfwSetDropCallback(window, [](GLFWwindow* window, int count, const char** paths) {
		auto* mgr = static_cast<LibraryManager*>(glfwGetWindowUserPointer(window));
		std::vector<std::filesystem::path> dropped;
		dropped.reserve(count);

		for (int i = 0; i < count; i++) {
			dropped.emplace_back(paths[i]);
		}

		mgr->importFiles(dropped);
	});

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::println("Could not initialize GLAD.\n");
		return -1;
	}

	state.playIcon = loadTextureFromResource("textures/play.png");
	state.playIconHovered = loadTextureFromResource("textures/play2.png");
	state.pauseIcon = loadTextureFromResource("textures/pause.png");
	state.pauseIconHovered = loadTextureFromResource("textures/pause2.png");
	state.volumeIcon = loadTextureFromResource("textures/volume.png");
	state.nextIcon = loadTextureFromResource("textures/next.png");
	state.repeatIcon = loadTextureFromResource("textures/repeat.png");
	state.shuffleIcon = loadTextureFromResource("textures/shuffle.png");
	state.searchIcon = loadTextureFromResource("textures/search.png");
	state.refreshIcon = loadTextureFromResource("textures/refresh.png");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	double lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		const double currentTime = glfwGetTime();
		const double targetTime = 1.0 / 60.0;

		const auto downloadStatus = downloader.getDownloadStatus();
		const bool isDownloading = downloadStatus == Downloader::DownloadStatus::Downloading;

		if ((player.isPlaying() || isDownloading) && (currentTime - lastTime < targetTime)) {
			const double remaining = targetTime - (currentTime - lastTime);
			glfwWaitEventsTimeout(remaining);
		}
		else {
			glfwWaitEvents();
		}
		lastTime = glfwGetTime();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Docking
		{
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
			}
			ImGui::EndMenuBar();

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

		ImGui::PushStyleColor(ImGuiCol_WindowBg,
			ImVec4(state.playerColor[0], state.playerColor[1], state.playerColor[2], state.playerColor[3]));

		ImGui::Begin("Player");
		const float curPos = ImGui::GetCursorPosX();

		// Song Name
		if (manager.selectedIndex >= 0 || manager.selectedPlaylist >= 0) {
			switch (manager.playingMode)
			{
			case LibraryManager::PlayingMode::None:
				ImGui::Text("%s", utils::toUtf8(manager.mSongs[manager.selectedIndex].stem()).c_str());
				break;
			case LibraryManager::PlayingMode::Queue:
				ImGui::Text("%s", utils::toUtf8(manager.mQueue[manager.selectedIndex].stem()).c_str());
				break;
			case LibraryManager::PlayingMode::History:
				ImGui::Text("%s", utils::toUtf8(manager.mHistory[manager.selectedIndex].stem()).c_str());
				break;
			case LibraryManager::PlayingMode::Playlist:
				const Playlist& selectedPlaylist = manager.mPlaylists[manager.selectedPlaylist];
				ImGui::Text("%s", utils::toUtf8(selectedPlaylist.songs[selectedPlaylist.selectedIndex].stem()).c_str());
				break;
			}

			ImGui::SameLine();
			ImGui::SetCursorPosX(curPos);
		}

		// Play Button
		const float size = std::clamp(ImGui::GetWindowSize().x * 0.04f, 32.0f, 48.0f);
		const float availX = ImGui::GetContentRegionAvail().x;
		const float offX = (availX - size) * 0.5f;
		if (offX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);
		const ImVec4 color(0, 0, 0, 0);
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
		const ImVec2 pos = ImGui::GetCursorScreenPos();
		const ImVec2 sizeVec(size, size);
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
		const ImVec2 playButPos = ImGui::GetCursorPos();
		if (ImGui::ImageButton("PlayButton", ImTextureRef((ImTextureID)tex), ImVec2(size, size))) {
			paused ? player.resume() : player.pause();
		}
		ImGui::SameLine();

		// Next Button
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 7.0f);
		const ImVec2 nextButCurPos = ImGui::GetCursorPos();
		if (ImGui::ImageButton("NextButton", ImTextureRef((ImTextureID)state.nextIcon), ImVec2(64.0f, 64.0f))) {
			utils::playNextSong(state, manager, player);
		}
		ImGui::SameLine();

		// Prev Button
		ImGui::SetCursorPos(ImVec2(playButPos.x - size - (64.0f * 0.5f), playButPos.y - 7.0f));
		const ImVec2 prevButCurPos = ImGui::GetCursorPos();
		if (ImGui::ImageButton("PrevButton", ImTextureRef((ImTextureID)state.nextIcon), ImVec2(64.0f, 64.0f), ImVec2(1, 0), ImVec2(0, 1))) {
			utils::playPrevSong(state, manager, player);
		}
		ImGui::SameLine();

		ImGui::SetCursorPos(ImVec2(prevButCurPos.x - 32.0f, prevButCurPos.y + 20.0f));
		if (ImGui::ImageButton("ShuffleButton", ImTextureRef((ImTextureID)state.shuffleIcon), ImVec2(24.0f, 24.0f))) {
			manager.mShuffleEnabled = !manager.mShuffleEnabled;
			manager.buildPlayOrder(manager.mShuffleEnabled);
			manager.setPlaylistsShuffle(manager.mShuffleEnabled);
		}

		ImGui::SameLine();

		// Repeat Button
		ImGui::SetCursorPos(ImVec2(nextButCurPos.x + 64.0f, nextButCurPos.y + 20.0f));
		if (ImGui::ImageButton("RepeatButton", ImTextureRef((ImTextureID)state.repeatIcon), ImVec2(24.0f, 24.0f))) {
			switch (state.repeatState) {
			case UIState::RepeatState::Off: state.repeatState = UIState::RepeatState::Once; break;
			case UIState::RepeatState::Once: state.repeatState = UIState::RepeatState::Always; break;
			case UIState::RepeatState::Always: state.repeatState = UIState::RepeatState::Off; break;
			}
		}
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

		ImGui::Text(leftTime.c_str());
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, -1.0f));
		ImGui::PushItemWidth(barWidth);

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

		ImGui::Text(rightTime.c_str());
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

		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

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

		// New Playlist Window
		if (state.playlistWindowOpen) {
			ImGui::SetNextWindowSize(ImVec2(350, 180));

			ImGui::Begin("New Playlist", &state.playlistWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
			const float windowWidth = ImGui::GetWindowSize().x;
			const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);

			const char* labelText = "Playlist Name";
			const float labelWidth = ImGui::CalcTextSize(labelText).x;
			ImGui::SetCursorPosX((windowWidth - labelWidth) * 0.5f);
			ImGui::Text(labelText);
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

		// Download Window
		if (state.downloadWindowOpen) {
			ImGui::SetNextWindowSize(ImVec2(350, 180));

			ImGui::Begin("Download a song", &state.downloadWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

			const float windowWidth = ImGui::GetWindowSize().x;
			const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);

			const char* labelText = "URL";
			const float labelWidth = ImGui::CalcTextSize(labelText).x;
			ImGui::SetCursorPosX((windowWidth - labelWidth) * 0.5f);
			ImGui::Text(labelText);
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

			ImGui::Text(statusText.c_str());

			ImGui::End();
		}

		// Options Window
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
			if (ImGui::Button("Reset to default##_playerBck")) {
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

		static bool wasPlaying = false;
		bool playing = player.isPlaying();
		if (wasPlaying && !playing && player.hasFinished()) {
			utils::playNextSong(state, manager, player);
		}
		wasPlaying = playing;

		if (downloadStatus == Downloader::DownloadStatus::Success) {
			manager.refreshSongs();
		}

		ImGui::Render();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}