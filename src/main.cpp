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
#include "Textures.h"

int main() {
	LibraryManager manager;
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

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::println("Could not initialize GLAD.\n");
		return -1;
	}

	const GLuint playIcon = loadTextureFromMemory(play, playLen);
	const GLuint playIconHovered = loadTextureFromMemory(play2, play2Len);
	const GLuint pauseIcon = loadTextureFromMemory(pause, pauseLen);
	const GLuint pauseIconHovered = loadTextureFromMemory(pause2, pause2Len);
	const GLuint volumeIcon = loadTextureFromMemory(volume, volumeLen);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

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

		// Play Button
		float size = ImGui::GetWindowSize().x * 0.04f;
		size = std::clamp(size, 32.0f, 48.0f);
		ImGui::SetCursorPosX((ImGui::GetWindowSize().x - size) * 0.5f);
		ImGui::SetCursorPosY((ImGui::GetWindowSize().y - size) * 0.5f - 20.0f);
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
			tex = hovered ? pauseIconHovered : pauseIcon;
		}
		else {
			tex = hovered ? playIconHovered : playIcon;
		}
		if (ImGui::ImageButton("PlayButton", ImTextureRef((ImTextureID)tex), ImVec2(size, size))) {
			if (paused) player.resume();
			else player.pause();
		}
		ImGui::PopStyleColor(3);

		// Progress Bar
		double current = player.getCurrentTime();
		double total = player.getTotalTime();
		float progress = (total > 0.0f) ? float(current / total) : 0.0f;
		const std::string leftTime = formatTime(current);
		const std::string rightTime = formatTime(total);
		
		const ImVec2 playButPosX = ImGui::GetItemRectMin();
		const float fourth = ImGui::GetContentRegionAvail().x / 4;
		const float cursorX = playButPosX.x - ImGui::CalcTextSize(leftTime.c_str()).x - fourth;
		ImGui::SetCursorPosX(cursorX + fourth * 0.5f);
		float y = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(y + 10.0f);

		ImGui::Text(leftTime.c_str());
		ImGui::SameLine();

		y = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(y + 2.0f);
		
		const float barWidth = fourth + ImGui::GetContentRegionAvail().x * 0.5f - ImGui::CalcTextSize(rightTime.c_str()).x;
		ImGui::SetNextItemWidth(barWidth - (fourth * 0.5f) * 2.0f);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, -1.0f));
		if (ImGui::SliderFloat("##SongBar", &progress, 0.0f, 1.0f, "")) {
			const double newTime = progress * total;
			player.seek(newTime);
		}
		ImGui::PopStyleVar(1);
		ImGui::SameLine();

		ImGui::SetCursorPosY(y);
		ImGui::Text(rightTime.c_str());
		ImGui::SameLine();

		// Volume Bar
		float x = ImGui::GetCursorPosX();
		ImGui::SetCursorPosX(x + ImGui::GetContentRegionAvail().x - 200.0f);
		ImGui::Image(ImTextureRef((ImTextureID)volumeIcon), ImVec2(16.0f, 16.0f));
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100.0f);
		y = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(y + 2.0f);
		static float volume = 1.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, -1.0f));
		if (ImGui::SliderFloat("##VolumeBar", &volume, 0.0f, 1.0f, "")) {
			player.setVolume(volume);
		}
		ImGui::PopStyleVar(1);
		ImGui::SameLine();

		ImGui::Text("%.0f", volume * 100.0f);

		ImGui::End();

		// Song List
		static int selectedIndex = -1;
		static int popupIndex = -1;
		ImGui::Begin("Songs");
		ImGui::BeginChild("SongList", ImVec2(0, 0), true);
		
		for (int i = 0; i < manager.mSongs.size(); i++) {
			const auto& song = manager.mSongs[i];

			if (ImGui::Selectable(song.stem().string().c_str(), selectedIndex == i)) {
				selectedIndex = i;
				player.play(song.string());
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
				ImGui::OpenPopup("SongContextMenu");
				popupIndex = i;
			}
		}

		if (ImGui::BeginPopup("SongContextMenu")) {
			if (ImGui::MenuItem("Delete")) {
				manager.erase(manager.mSongs[popupIndex]);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::EndChild();
		ImGui::End();

		static bool wasPlaying = false;
		bool playing = player.isPlaying();
		if (wasPlaying && !playing && player.hasFinished()) {
			playNextSong(manager, player, selectedIndex);
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