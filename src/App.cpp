#include "App.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

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
	mUI.draw(mState, mSettings, mManager, mPlayer, mDownloader, mDownloadStatus);
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