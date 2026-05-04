#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "UIState.h"
#include "SettingsManager.h"
#include "LibraryManager.h"
#include "AudioPlayer.h"
#include "Downloader.h"

#define OGL_VERSION_MAJOR 3
#define OGL_VERSION_MINOR 3
#define IMGUI_GLSL_VERSION "#version 330"

class App {
public:
	App();
	~App();

	int run();

private:
	bool init();

	bool initWindow();
	bool initOpenGL();
	void loadTextures();
	void initImGui();

	void mainLoop();

	void beginFrame();
	void renderUI();
	void updatePlayback();
	void updateDownloads();
	void endFrame();

	void shutdown();

private:
	GLFWwindow* mWindow = nullptr;

	UIState mState;
	SettingsManager mSettings;
	LibraryManager mManager;
	Downloader mDownloader;
	AudioPlayer mPlayer;

	Downloader::DownloadStatus mDownloadStatus;
};