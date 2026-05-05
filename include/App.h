#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "UIState.h"
#include "SettingsManager.h"
#include "LibraryManager.h"
#include "AudioPlayer.h"
#include "Downloader.h"
#include "AppUI.h"

#define OGL_VERSION_MAJOR 3
#define OGL_VERSION_MINOR 3
#define IMGUI_GLSL_VERSION "#version 330"

class App {
public:
	App();
	~App();

	App(const App&) = delete;
	App& operator=(const App&) = delete;
	App(App&&) = delete;
	App& operator=(App&&) = delete;

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
	AppUI mUI;

	Downloader::DownloadStatus mDownloadStatus = Downloader::DownloadStatus::Idle;
};