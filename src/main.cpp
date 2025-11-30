#include <print>
#include <format>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "LibraryManager.h"
#include "AudioPlayer.h"
#include "TextureLoader.h"

std::string formatTime(double& seconds) {
	int s = static_cast<int>(seconds);
	int min = s / 60;
	int sec = s % 60;
	return std::format("{}:{:02}", min, sec);
}

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

	GLuint playIcon = loadTexture(std::string(TEXTURE_DIR) + "/play.png");
	GLuint playIconHovered = loadTexture(std::string(TEXTURE_DIR) + "/play2.png");
	GLuint pauseIcon = loadTexture(std::string(TEXTURE_DIR) + "/pause.png");
	GLuint pauseIconHovered = loadTexture(std::string(TEXTURE_DIR) + "/pause2.png");

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
		size = std::clamp(size, 32.0f, 64.0f);
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
		float progress = (total > 0.0) ? float(current / total) : 0.0f;
		std::string leftTime = formatTime(current);
		std::string rightTime = formatTime(total);
		
		ImVec2 playButPosX = ImGui::GetItemRectMin();
		ImVec2 playButPosY = ImGui::GetItemRectMax();
		float cursorX = playButPosX.x - ImGui::CalcTextSize(leftTime.c_str()).x - ImGui::GetContentRegionAvail().x / 4;
		ImGui::SetCursorPosX(cursorX);

		ImGui::Text(leftTime.c_str());
		ImGui::SameLine();
		ImGui::SetNextItemWidth(cursorX + ImGui::CalcTextSize(leftTime.c_str()).x + ImGui::CalcTextSize(leftTime.c_str()).x + ImGui::GetContentRegionAvail().x / 4);

		if (ImGui::SliderFloat("##SongBar", &progress, 0.0f, 1.0f, "")) {
			double newTime = progress * total;
			player.seek(newTime);
		}
		ImGui::SameLine();

		ImGui::Text(rightTime.c_str());

		ImGui::End();

		static int selectedIndex = -1;
		ImGui::Begin("Songs");
		ImGui::BeginChild("SongList", ImVec2(0, 0), true);
		
		for (int i = 0; i < manager.mSongs.size(); i++) {
			const auto& song = manager.mSongs[i];

			if (ImGui::Selectable(song.stem().string().c_str(), selectedIndex == i)) {
				selectedIndex = i;
				player.play(song.string());
			}
		}

		ImGui::EndChild();
		ImGui::End();

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