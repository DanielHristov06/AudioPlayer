#include "App.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include "TextureLoader.h"
#include "Utils.h"
#include "Logger.h"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

App::App() {
	mSettings.load(mState);
}

App::~App() {
	shutdown();
}

int App::run() {
	if (!init()) {
		return -1;
	}

	mainLoop();
	return 0;
}

bool App::init() {
	if (!initWindow()) {
		return false;
	}

	if (!initOpenGL()) {
		return false;
	}

	loadTextures();

	initImGui();

	return true;
}

bool App::initWindow() {
	glfwSetErrorCallback([](int code, const char* description) {
		Logger::get().log(Logger::Level::Error, "GLFW error {}: {}", code, description ? description : "Unkown error");
	});

	if (!glfwInit()) {
		Logger::get().log(Logger::Level::Error, "Could not initialize GLFW.\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OGL_VERSION_MAJOR);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OGL_VERSION_MINOR);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	mWindow = glfwCreateWindow(1280, 720, "Audio Player", NULL, NULL);
	if (!mWindow) {
		Logger::get().log(Logger::Level::Error, "Failed to create a window.\n");
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(mWindow);

	glfwSetFramebufferSizeCallback(mWindow, [](GLFWwindow* window, int width, int height) {
		glViewport(0, 0, width, height);
	});

	glfwSetWindowUserPointer(mWindow, &mManager);
	glfwSetDropCallback(mWindow, [](GLFWwindow* window, int count, const char** paths) {
		LibraryManager* mgr = static_cast<LibraryManager*>(glfwGetWindowUserPointer(window));
		std::vector<std::filesystem::path> dropped;
		dropped.reserve(count);

		for (int i = 0; i < count; i++) {
			dropped.emplace_back(paths[i]);
		}

		mgr->importFiles(dropped);
	});

	return true;
}

bool App::initOpenGL() {
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::get().log(Logger::Level::Error, "Could not initialize GLAD.\n");
		return false;
	}

	return true;
}

void App::loadTextures() {
	mState.playIcon = loadTextureFromResource("textures/play.png");
	mState.playIconHovered = loadTextureFromResource("textures/play2.png");
	mState.pauseIcon = loadTextureFromResource("textures/pause.png");
	mState.pauseIconHovered = loadTextureFromResource("textures/pause2.png");
	mState.volumeIcon = loadTextureFromResource("textures/volume.png");
	mState.nextIcon = loadTextureFromResource("textures/next.png");
	mState.repeatIcon = loadTextureFromResource("textures/repeat.png");
	mState.shuffleIcon = loadTextureFromResource("textures/shuffle.png");
	mState.searchIcon = loadTextureFromResource("textures/search.png");
	mState.refreshIcon = loadTextureFromResource("textures/refresh.png");
	mState.forwardIcon = loadTextureFromResource("textures/forward.png");
}

void App::initImGui() {
	static const std::string imguiIniPath = (mManager.getMainDir() / "imgui.ini").string();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = imguiIniPath.c_str();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForOpenGL(mWindow, true);
	ImGui_ImplOpenGL3_Init(IMGUI_GLSL_VERSION);
}

void App::mainLoop() {
	double lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(mWindow)) {
		const double currentTime = glfwGetTime();
		const double targetTime = 1.0 / 60.0;

		mDownloadStatus = mDownloader.getDownloadStatus();
		const bool isDownloading = mDownloadStatus == Downloader::DownloadStatus::Downloading;

		if ((mPlayer.isPlaying() || isDownloading) && (currentTime - lastTime < targetTime)) {
			const double remaining = targetTime - (currentTime - lastTime);
			glfwWaitEventsTimeout(remaining);
		}
		else {
			glfwWaitEvents();
		}
		lastTime = glfwGetTime();

		beginFrame();
		renderUI();
		updatePlayback();
		updateDownloads();
		endFrame();
	}
}

void App::beginFrame() {
	if (ImGui::GetCurrentContext()) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
}

void App::renderUI() {
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
				mManager.import();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("New Playlist")) {
				mState.playlistWindowOpen = !mState.playlistWindowOpen;
				if (mState.playlistWindowOpen) mState.playlistName[0] = '\0';
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Download", nullptr, false, mDownloader.isReady())) {
				mState.downloadWindowOpen = !mState.downloadWindowOpen;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Open in explorer")) {
				utils::openInExplorer(mManager.getMainDir());
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Options")) {
				mState.optionsWindowOpen = !mState.optionsWindowOpen;
			}
		}
		ImGui::EndMenuBar();

		const ImGuiID dockspaceId = ImGui::GetID("Dockspace");

		if (mState.firstTime || mState.rebuildDock) {
			mState.firstTime = false;
			mState.rebuildDock = false;

			ImGui::DockBuilderRemoveNode(dockspaceId);
			ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

			ImGuiID dockMain = dockspaceId;
			ImGuiID dockBottom{}, dockSongs{};

			const ImGuiDir songsPos = mState.selectedSongsPos == 0 ? ImGuiDir_Left : ImGuiDir_Right;

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
		ImVec4(mState.queueBckColor[0], mState.queueBckColor[1], mState.queueBckColor[2], mState.queueBckColor[3]));
	ImGui::Begin("Other");

	const bool bothVisible = mState.queueEnabled && mState.historyEnabled;
	const float otherSpacing = ImGui::GetStyle().ItemSpacing.x;
	const float availWidth = ImGui::GetContentRegionAvail().x;
	const float childWidth = bothVisible ? (availWidth * 0.5f - otherSpacing * 0.5f) : availWidth;
	const float childHeight = ImGui::GetContentRegionAvail().y;

	if (mState.queueEnabled) {
		ImGui::BeginChild("##QueueChild", ImVec2(childWidth, childHeight), true);

		ImGui::Text("Queue:");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Clear queue").x - 10.0f + ImGui::GetCursorPosX());

		if (ImGui::Button("Clear Queue")) {
			mManager.mQueue.clear();
			mManager.playingMode = LibraryManager::PlayingMode::None;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (!mManager.mQueue.empty()) {
			for (size_t i = 0; i < mManager.mQueue.size(); i++) {
				const auto& song = mManager.mQueue[i];
				utils::renderSongSelectable(song, LibraryManager::PlayingMode::Queue, i, -1, "##queue_" + std::to_string(i), "QueueSongContextMenu", mState, mManager, mPlayer);
			}

			if (ImGui::BeginPopup("QueueSongContextMenu")) {
				const fs::path& popupSong = mManager.mQueue[mState.popupIndex];

				if (ImGui::MenuItem("Play")) {
					mPlayer.play(popupSong);
					mManager.addSongToHistory(popupSong);
					mState.currentlyPlayingPath = popupSong;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Remove from queue")) {
					mManager.removeSongFromQueue(popupSong);
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Add to playlist")) {
					for (auto& playlist : mManager.mPlaylists) {
						const bool alreadyIn = mManager.isSongInPlaylist(popupSong, playlist);
						if (ImGui::MenuItem(playlist.name.c_str(), nullptr, false, !alreadyIn)) {
							mManager.addSongToPlaylist(playlist, popupSong);
						}
					}

					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}
		}

		ImGui::EndChild();
	}

	if (mState.historyEnabled) {
		if (bothVisible) ImGui::SameLine();

		ImGui::BeginChild("##HistoryChild", ImVec2(childWidth, childHeight), true);

		ImGui::Text("History:");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Clear History").x - 10.0f + ImGui::GetCursorPosX());

		if (ImGui::Button("Clear History")) {
			mManager.mHistory.clear();
			mManager.playingMode = LibraryManager::PlayingMode::None;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (!mManager.mHistory.empty()) {
			for (size_t i = 0; i < mManager.mHistory.size(); i++) {
				const auto& song = mManager.mHistory[i];
				utils::renderSongSelectable(song, LibraryManager::PlayingMode::History, i, -1, "##ll_" + std::to_string(i), "HistorySongContextMenu", mState, mManager, mPlayer);
			}

			if (ImGui::BeginPopup("HistorySongContextMenu")) {
				const fs::path& popupSong = mManager.mHistory[mState.popupIndex];

				if (ImGui::MenuItem("Play")) {
					mPlayer.play(popupSong);
					mManager.addSongToHistory(popupSong);
					mState.currentlyPlayingPath = popupSong;
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Remove from History")) {
					mManager.removeSongFromHistory(popupSong);
					ImGui::CloseCurrentPopup();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Add to playlist")) {
					for (auto& playlist : mManager.mPlaylists) {
						const bool alreadyIn = mManager.isSongInPlaylist(popupSong, playlist);
						if (ImGui::MenuItem(playlist.name.c_str(), nullptr, false, !alreadyIn)) {
							mManager.addSongToPlaylist(playlist, popupSong);
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
		ImVec4(mState.playerColor[0], mState.playerColor[1], mState.playerColor[2], mState.playerColor[3]));

	ImGui::Begin("Player");
	const float curPos = ImGui::GetCursorPosX();

	// Song Name
	if (mManager.selectedIndex >= 0 || mManager.selectedPlaylist >= 0) {
		switch (mManager.playingMode)
		{
		case LibraryManager::PlayingMode::None:
			ImGui::Text("%s", utils::toUtf8(mManager.mSongs[mManager.selectedIndex].stem()).c_str());
			break;
		case LibraryManager::PlayingMode::Queue:
			ImGui::Text("%s", utils::toUtf8(mManager.mQueue[mManager.selectedIndex].stem()).c_str());
			break;
		case LibraryManager::PlayingMode::History:
			ImGui::Text("%s", utils::toUtf8(mManager.mHistory[mManager.selectedIndex].stem()).c_str());
			break;
		case LibraryManager::PlayingMode::Playlist:
			const Playlist& selectedPlaylist = mManager.mPlaylists[mManager.selectedPlaylist];
			ImGui::Text("%s", utils::toUtf8(selectedPlaylist.songs[selectedPlaylist.selectedIndex].stem()).c_str());
			break;
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
	const float mPlayerSpacing = 12.0f;

	const float rowHeight = bigSize;

	const float totalWidth =
		smallSize +
		mPlayerSpacing + smallSize +
		mPlayerSpacing + bigSize +
		mPlayerSpacing + playSize +
		mPlayerSpacing + bigSize +
		mPlayerSpacing + bigSize +
		mPlayerSpacing + smallSize;

	const float startX = (ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f;

	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

	const ImVec2 rowStart = ImGui::GetCursorPos();

	auto CenteredIconButton = [&](const char* id, ImTextureID texture, float size, ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1)) {
		const float yOffset = (rowHeight - size) * 0.5f;
		ImGui::SetCursorPosY(rowStart.y + yOffset - 10.0f);
		return ImGui::ImageButton(id, ImTextureRef(texture), ImVec2(size, size), uv0, uv1);
		};

	// Shuffle Button
	if (CenteredIconButton("Shuffle", (ImTextureID)mState.shuffleIcon, smallSize)) {
		mManager.mShuffleEnabled = !mManager.mShuffleEnabled;
		mManager.buildPlayOrder(mManager.mShuffleEnabled);
		mManager.setPlaylistsShuffle(mManager.mShuffleEnabled);
	}

	ImGui::SameLine(0.0f, mPlayerSpacing);

	// Backward Button
	if (CenteredIconButton("Backward", (ImTextureID)mState.forwardIcon, smallSize, ImVec2(1, 0), ImVec2(0, 1))) {
		mPlayer.backward(mState.backwardSkip);
	}

	ImGui::SameLine(0.0f, mPlayerSpacing);

	// Previous Button
	if (CenteredIconButton("Previous", (ImTextureID)mState.nextIcon, bigSize, ImVec2(1, 0), ImVec2(0, 1))) {
		utils::playPrevSong(mState, mManager, mPlayer);
	}

	ImGui::SameLine(0.0f, mPlayerSpacing);

	// Play Button
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const ImVec2 sizeVec(playSize, playSize);
	const ImRect rect(pos, ImVec2(pos.x + sizeVec.x, pos.y + sizeVec.y));
	bool hovered = rect.Contains(ImGui::GetIO().MousePos);
	GLuint tex{};
	const bool paused = mPlayer.isPaused();
	if (!paused) {
		tex = hovered ? mState.pauseIconHovered : mState.pauseIcon;
	}
	else {
		tex = hovered ? mState.playIconHovered : mState.playIcon;
	}

	if (CenteredIconButton("PlayButton", tex, playSize)) {
		paused ? mPlayer.resume() : mPlayer.pause();
	}

	ImGui::SameLine(0.0f, mPlayerSpacing);

	// Next Button
	if (CenteredIconButton("Next", (ImTextureID)mState.nextIcon, bigSize)) {
		utils::playNextSong(mState, mManager, mPlayer);
	}

	ImGui::SameLine(0.0f, mPlayerSpacing);

	// Forward Button
	if (CenteredIconButton("Forward", (ImTextureID)mState.forwardIcon, smallSize)) {
		mPlayer.forward(mState.forwardSkip);
	}

	ImGui::SameLine(0.0f, mPlayerSpacing);

	// Repeat Button
	if (CenteredIconButton("RepeatButton", (ImTextureID)mState.repeatIcon, smallSize)) {
		switch (mState.repeatState) {
		case UIState::RepeatState::Off:
			mState.repeatState = UIState::RepeatState::Once;
			break;

		case UIState::RepeatState::Once:
			mState.repeatState = UIState::RepeatState::Always;
			break;

		case UIState::RepeatState::Always:
			mState.repeatState = UIState::RepeatState::Off;
			break;
		}
	}

	ImGui::SetCursorPosY(rowStart.y + rowHeight);

	ImGui::PopStyleColor(3);

	// Progress Bar
	double current = mPlayer.getCurrentTime();
	double total = mPlayer.getTotalTime();
	float progress = (total > 0.0f) ? float(current / total) : 0.0f;
	if (mState.isSeeking) progress = mState.seekPreview;
	const double displayTime = mState.isSeeking ? (mState.seekPreview * total) : current;
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
	ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);

	ImGui::SliderFloat("##SongBar", &progress, 0.0f, 1.0f, "");

	if (ImGui::IsItemActive()) {
		mState.isSeeking = true;
		mState.seekPreview = progress;
	}
	if (ImGui::IsItemDeactivatedAfterEdit()) {
		mPlayer.seek(mState.seekPreview * total);
		mState.isSeeking = false;
	}

	ImGui::PopStyleVar();
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::Text(rightTime.c_str());
	ImGui::SameLine();

	// Volume Bar
	const float x = ImGui::GetCursorPosX();
	ImGui::SetCursorPosX(x + ImGui::GetContentRegionAvail().x - 200.0f);
	ImGui::Image(ImTextureRef((ImTextureID)mState.volumeIcon), ImVec2(16.0f, 16.0f));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(100.0f);
	const float y = ImGui::GetCursorPosY();
	ImGui::SetCursorPosY(y + 2.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, -1.0f));
	if (ImGui::SliderFloat("##VolumeBar", &mState.volume, 0.0f, 1.0f, "")) {
		mPlayer.setVolume(mState.volume);
	}
	ImGui::PopStyleVar(1);
	ImGui::SameLine();

	ImGui::Text("%.0f", mState.volume * 100.0f);

	ImGui::End();

	ImGui::PopStyleColor();

	ImGui::PushStyleColor(ImGuiCol_WindowBg,
		ImVec4(mState.songsColor[0], mState.songsColor[1], mState.songsColor[2], mState.songsColor[3]));

	// Song List
	ImGui::Begin("Songs");
	ImGui::BeginChild("SongList", ImVec2(0, 0), true);

	if (!mState.searchOpen) {
		ImGui::Text("Song List:");
	}
	else {
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
		ImGui::InputText("##SearchBar", mState.searchQuery, IM_ARRAYSIZE(mState.searchQuery));
	}

	ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60.0f + ImGui::GetCursorPosX());

	ImGui::PushStyleColor(ImGuiCol_Button, transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);

	if (ImGui::ImageButton("Search", ImTextureRef((ImTextureID)mState.searchIcon), ImVec2(18.0f, 18.0f))) {
		mState.searchOpen = !mState.searchOpen;
		if (!mState.searchOpen) mState.searchQuery[0] = '\0';
	}

	ImGui::SameLine();

	if (ImGui::ImageButton("Refresh", ImTextureRef((ImTextureID)mState.refreshIcon), ImVec2(18.0f, 18.0f))) {
		if (!mManager.refreshing) mManager.refreshSongs();
	}

	ImGui::PopStyleColor(3);

	ImGui::Dummy(ImVec2(0.0f, 4.0f));
	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.0f, 4.0f));

	const std::string query = utils::toLower(mState.searchQuery);
	const bool isSearching = !query.empty();

	if (isSearching) {
		if (ImGui::CollapsingHeader("All Songs")) {
			for (size_t i = 0; i < mManager.mSongs.size(); i++) {
				const auto& song = mManager.mSongs[i];
				const std::string stem = utils::toLower(utils::toUtf8(song.stem()));
				if (stem.find(query) != std::string::npos) {
					utils::renderSongSelectable(song, LibraryManager::PlayingMode::None, i, -1, "##main_" + std::to_string(i), "SongContextMenu", mState, mManager, mPlayer);
				}
			}
		}
	}
	else {
		if (ImGui::CollapsingHeader("All Songs")) {
			for (size_t i = 0; i < mManager.mSongs.size(); i++) {
				const auto& song = mManager.mSongs[i];
				utils::renderSongSelectable(song, LibraryManager::PlayingMode::None, i, -1, "##main_" + std::to_string(i), "SongContextMenu", mState, mManager, mPlayer);
			}
		}
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (isSearching) {
		for (size_t p = 0; p < mManager.mPlaylists.size(); p++) {
			auto& playlist = mManager.mPlaylists[p];
			if (utils::toLower(playlist.name).find(query) != std::string::npos) {
				const bool headerOpen = ImGui::CollapsingHeader(playlist.name.c_str());
				const bool headerHovered = ImGui::IsItemHovered();

				if (headerOpen) {
					for (size_t i = 0; i < playlist.songs.size(); i++) {
						const auto& song = playlist.songs[i];
						utils::renderSongSelectable(song, LibraryManager::PlayingMode::Playlist, i, p, "##pl_" + std::to_string(p) + "_" + std::to_string(i), "SongContextMenu", mState, mManager, mPlayer);
					}

					if (headerHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
						ImGui::OpenPopup("PlaylistContextMenu");
						mState.popupPlaylistIndex = p;
					}
				}
			}
		}
	}
	else {
		for (size_t p = 0; p < mManager.mPlaylists.size(); p++) {
			auto& playlist = mManager.mPlaylists[p];
			const bool headerOpen = ImGui::CollapsingHeader(playlist.name.c_str());
			const bool headerHovered = ImGui::IsItemHovered();

			if (headerOpen) {
				for (size_t i = 0; i < playlist.songs.size(); i++) {
					const auto& song = playlist.songs[i];
					utils::renderSongSelectable(song, LibraryManager::PlayingMode::Playlist, i, p, "##pl_" + std::to_string(p) + "_" + std::to_string(i), "SongContextMenu", mState, mManager, mPlayer);
				}

				if (headerHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
					ImGui::OpenPopup("PlaylistContextMenu");
					mState.popupPlaylistIndex = p;
				}
			}
		}
	}

	if (ImGui::BeginPopup("SongContextMenu")) {
		const fs::path& popupSong = (mState.popupPlaylistIndex == -1)
			? mManager.mSongs[mState.popupIndex] : mManager.mPlaylists[mState.popupPlaylistIndex].songs[mState.popupIndex];

		if (ImGui::MenuItem("Play")) {
			mPlayer.play(popupSong);
			mManager.addSongToHistory(popupSong);
			mState.currentlyPlayingPath = popupSong;
			ImGui::CloseCurrentPopup();
		}

		if (mState.popupPlaylistIndex == -1) {
			if (ImGui::MenuItem("Delete")) {
				mManager.erase(mManager.mSongs[mState.popupIndex]);
				ImGui::CloseCurrentPopup();
			}
		}
		else {
			if (ImGui::MenuItem("Remove from playlist")) {
				mManager.removeSongFromPlaylist(mManager.mPlaylists[mState.popupPlaylistIndex], mState.popupIndex);
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Add to queue")) {
			mManager.addSongToQueue(popupSong);
		}

		if (ImGui::BeginMenu("Add to playlist")) {
			for (auto& playlist : mManager.mPlaylists) {
				const bool alreadyIn = mManager.isSongInPlaylist(popupSong, playlist);
				if (ImGui::MenuItem(playlist.name.c_str(), nullptr, false, !alreadyIn)) {
					mManager.addSongToPlaylist(playlist, popupSong);
				}
			}

			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("PlaylistContextMenu")) {
		if (ImGui::MenuItem("Delete playlist")) {
			mManager.removePlaylist(mManager.mPlaylists[mState.popupPlaylistIndex]);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::EndChild();
	ImGui::End();

	ImGui::PopStyleColor();

	// New Playlist Window
	if (mState.playlistWindowOpen) {
		ImGui::SetNextWindowSize(ImVec2(350, 180));

		ImGui::Begin("New Playlist", &mState.playlistWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
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
		ImGui::InputText("##NameInput", mState.playlistName, IM_ARRAYSIZE(mState.playlistName));
		ImGui::PopItemWidth();

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const float buttonWidth = 50.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalButtonWidth = (buttonWidth * 2) + spacing;
		ImGui::SetCursorPosX((windowWidth - totalButtonWidth) * 0.5f);
		if (ImGui::Button("Cancel")) {
			mState.playlistWindowOpen = false;
			mState.playlistName[0] = '\0';
		}
		ImGui::SameLine();

		if (ImGui::Button("Create")) {
			const std::string plName(mState.playlistName);
			mManager.createPlaylist(plName);
			mState.playlistName[0] = '\0';
		}

		ImGui::End();
	}

	// Download Window
	if (mState.downloadWindowOpen) {
		ImGui::SetNextWindowSize(ImVec2(350, 180));

		ImGui::Begin("Download a song", &mState.downloadWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		const float windowWidth = ImGui::GetWindowSize().x;
		const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);

		const char* labelText = "URL";
		const float labelWidth = ImGui::CalcTextSize(labelText).x;
		ImGui::SetCursorPosX((windowWidth - labelWidth) * 0.5f);
		ImGui::Text(labelText);
		ImGui::Dummy(widgetSpacing);

		const float inputWidth = windowWidth * 0.75f;
		ImGui::PushItemWidth(inputWidth);
		ImGui::InputText("##UrlInput", mState.url, IM_ARRAYSIZE(mState.url));
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::SetNextItemWidth(65.0f);
		ImGui::Combo("##Format", &mState.selectedFormat, mState.formats, IM_ARRAYSIZE(mState.formats));

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const float buttonWidth = 50.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalButtonWidth = (buttonWidth * 2) + spacing;
		ImGui::SetCursorPosX((windowWidth - totalButtonWidth) * 0.5f);
		if (ImGui::Button("Cancel")) {
			mState.downloadWindowOpen = false;
			mState.url[0] = '\0';
		}
		ImGui::SameLine();

		if (ImGui::Button("Download")) {
			std::string format = std::string(mState.formats[mState.selectedFormat]).substr(1);
			mDownloader.download(mState.url, mManager.getMusicDir(), format);
			mState.url[0] = '\0';
		}

		std::string statusText = "";

		switch (mDownloadStatus) {
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
	if (mState.optionsWindowOpen) {
		ImGui::Begin("Options", &mState.optionsWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse);
		const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);
		const float windowWidth = ImGui::GetWindowSize().x;

		ImGui::ColorEdit4("Queue Background Color", mState.queueBckColor);
		if (ImGui::Button("Reset to default##_queueBck")) {
			std::copy(std::begin(mState.queueDefaultBckColor), std::end(mState.queueDefaultBckColor), mState.queueBckColor);
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		ImGui::ColorEdit4("Player Background Color", mState.playerColor);
		if (ImGui::Button("Reset to default##_mPlayerBck")) {
			std::copy(std::begin(mState.playerDefaultColor), std::end(mState.playerDefaultColor), mState.playerColor);
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		ImGui::ColorEdit4("Songs Background Color", mState.songsColor);
		if (ImGui::Button("Reset to default##_songsBck")) {
			std::copy(std::begin(mState.songsDefaultColor), std::end(mState.songsDefaultColor), mState.songsColor);
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		if (ImGui::Combo("Song list position", &mState.selectedSongsPos, mState.songsPos, IM_ARRAYSIZE(mState.songsPos))) {
			mState.rebuildDock = true;
		}

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		ImGui::Checkbox("Enable queue", &mState.queueEnabled);
		ImGui::SameLine();
		ImGui::Checkbox("Enable history", &mState.historyEnabled);

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const double min = 0.1, max = 30.0;
		ImGui::DragScalar("Forward skip seconds", ImGuiDataType_Double, &mState.forwardSkip, 0.1, &min, &max, "%.2f");
		ImGui::DragScalar("Backward skip seconds", ImGuiDataType_Double, &mState.backwardSkip, 0.1, &min, &max, "%.2f");

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		const float buttonWidth = (ImGui::CalcTextSize("Cancel").x + ImGui::CalcTextSize("Apply").x) / 2.0f;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float totalButtonWidth = (buttonWidth * 2) + spacing;
		ImGui::SetCursorPosX((windowWidth - totalButtonWidth) * 0.5f);
		if (ImGui::Button("Cancel")) {
			mState.optionsWindowOpen = false;
		}
		ImGui::SameLine();

		if (ImGui::Button("Apply")) {
			mSettings.save(mState);
		}

		ImGui::End();
	}

	Logger::get().drawWindow();
}

void App::updatePlayback() {
	bool playing = mPlayer.isPlaying();
	if (mState.wasPlaying && !playing && mPlayer.hasFinished()) {
		utils::playNextSong(mState, mManager, mPlayer);
	}
	mState.wasPlaying = playing;
}

void App::updateDownloads() {
	if (mDownloadStatus == Downloader::DownloadStatus::Success) {
		mManager.refreshSongs();
	}
}

void App::endFrame() {
	if (ImGui::GetCurrentContext()) {
		ImGui::Render();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	glfwSwapBuffers(mWindow);
}

void App::shutdown() {
	if (ImGui::GetCurrentContext()) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	if (mWindow) {
		glfwDestroyWindow(mWindow);
		mWindow = nullptr;
	}

	glfwTerminate();
}