#include <print>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "LibraryManager.h"
#include "AudioPlayer.h"
#include "TextureLoader.h"
#include "Utils.h"

int main() {
	LibraryManager manager;
	AudioPlayer player;
	utils::UIState state;

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

		if (player.isPlaying() && (currentTime - lastTime < targetTime)) {
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
			}
			ImGui::EndMenuBar();

			const ImGuiID dockspaceId = ImGui::GetID("Dockspace");
			
			static bool firstTime = true;
			if (firstTime) {
				firstTime = false;

				ImGui::DockBuilderRemoveNode(dockspaceId);
				ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
				ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->Size);

				ImGuiID dockMain = dockspaceId;
				ImGuiID dockBottom{}, dockLeft{};

				ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.15f, &dockBottom, &dockMain);
				ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.20f, &dockLeft, &dockMain);
				ImGui::DockBuilderGetNode(dockBottom)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
				ImGui::DockBuilderGetNode(dockLeft)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
				ImGui::DockBuilderDockWindow("Player", dockBottom);
				ImGui::DockBuilderDockWindow("Songs", dockLeft);
				ImGui::DockBuilderFinish(dockspaceId);
			}

			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));

			ImGui::End();
		}

		ImGui::Begin("Player");
		const float curPos = ImGui::GetCursorPosX();

		// Song Name
		if (manager.selectedIndex >= 0 || manager.selectedPlaylist >= 0) {
			if (!manager.isPlayingFomPlaylist) {
				ImGui::Text(manager.mSongs[manager.selectedIndex].stem().string().c_str());
			}
			else {
				Playlist selectedPlaylist = manager.mPlaylists[manager.selectedPlaylist];
				ImGui::Text(selectedPlaylist.songs[selectedPlaylist.selectedIndex].stem().string().c_str());
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
		if (ImGui::ImageButton("NextButton", ImTextureRef((ImTextureID)state.nextIcon), ImVec2(64.0f, 64.0f))) {
			utils::playNextSong(state, manager, player);
		}

		// Prev Button
		ImGui::SetCursorPos(ImVec2(playButPos.x - size - (64.0f * 0.5f), playButPos.y - 7.0f));
		if (ImGui::ImageButton("PrevButton", ImTextureRef((ImTextureID)state.nextIcon), ImVec2(64.0f, 64.0f), ImVec2(1, 0), ImVec2(0, 1))) {
			utils::playPrevSong(state, manager, player);
		}
		ImGui::PopStyleColor(3);

		// Progress Bar
		double current = player.getCurrentTime();
		double total = player.getTotalTime();
		float progress = (total > 0.0f) ? float(current / total) : 0.0f;
		const std::string leftTime = utils::formatTime(current);
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
		if (ImGui::SliderFloat("##SongBar", &progress, 0.0f, 1.0f, "")) {
			const double newTime = progress * total;
			player.seek(newTime);
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

		// Song List
		ImGui::Begin("Songs");
		ImGui::BeginChild("SongList", ImVec2(0, 0), true);

		ImGui::Text("Song List:");
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		
		for (int i = 0; i < static_cast<int>(manager.mSongs.size()); i++) {
			const auto& song = manager.mSongs[i];

			const std::string mainLabel = song.stem().string() + "##main_" + std::to_string(i);
			if (ImGui::Selectable(mainLabel.c_str(), song == state.currentlyPlayingPath)) {
				manager.selectedIndex = i;
				manager.isPlayingFomPlaylist = false;
				if (manager.selectedPlaylist != -1) {
					manager.mPlaylists[manager.selectedPlaylist].selectedIndex = -1;
					manager.selectedPlaylist = -1;
				}
				if (song != state.currentlyPlayingPath) {
					player.play(song.string());
					state.currentlyPlayingPath = song;
				}
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("SongContextMenu");
				state.popupIndex = i;
				state.popupPlaylistIndex = -1;
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		for (int p = 0; p < static_cast<int>(manager.mPlaylists.size()); p++) {
			auto& playlist = manager.mPlaylists[p];
			const bool headerOpen = ImGui::CollapsingHeader(playlist.name.c_str());
			const bool headerHovered = ImGui::IsItemHovered();

			if (headerOpen) {
				for (int i = 0; i < static_cast<int>(playlist.songs.size()); i++) {
					const auto& song = playlist.songs[i];

					const std::string plLabel = song.stem().string() + "##pl_" + std::to_string(p) + "_" + std::to_string(i);
					if (ImGui::Selectable(plLabel.c_str(), song == state.currentlyPlayingPath)) {
						manager.selectedIndex = -1;
						manager.isPlayingFomPlaylist = true;
						manager.selectedPlaylist = p;
						playlist.selectedIndex = i;
						if (song != state.currentlyPlayingPath) {
							player.play(song.string());
							state.currentlyPlayingPath = song;
						}
					}

					if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
						ImGui::OpenPopup("SongContextMenu");
						state.popupIndex = i;
						state.popupPlaylistIndex = p;
					}
				}
			}

			if (headerHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("PlaylistContextMenu");
				state.popupPlaylistIndex = p;
			}
		}

		if (ImGui::BeginPopup("SongContextMenu")) {
			const fs::path& popupSong = (state.popupPlaylistIndex == -1)
				? manager.mSongs[state.popupIndex] : manager.mPlaylists[state.popupPlaylistIndex].songs[state.popupIndex];

			if (ImGui::MenuItem("Play")) {
				player.play(popupSong.string());
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
				//state.playlistWindowOpen = false;
				//state.playlistName[0] = '\0';
				manager.download("https://www.youtube.com/watch?v=4vwse_8jC5k");
			}
			ImGui::SameLine();

			if (ImGui::Button("Create")) {
				const std::string plName(state.playlistName);
				manager.createPlaylist(plName);
				state.playlistName[0] = '\0';
			}

			ImGui::End();
		}

		static bool wasPlaying = false;
		bool playing = player.isPlaying();
		if (wasPlaying && !playing && player.hasFinished()) {
			utils::playNextSong(state, manager, player);
		}
		wasPlaying = playing;

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