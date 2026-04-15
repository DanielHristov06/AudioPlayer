#pragma once
#include "LibraryManager.h"
#include "UIState.h"
#include <filesystem>

namespace fs = std::filesystem;

class SettingsManager {
public:
	SettingsManager();

	SettingsManager(const SettingsManager&) = delete;
	SettingsManager& operator=(const SettingsManager&) = delete;
	SettingsManager(SettingsManager&&) = delete;
	SettingsManager& operator=(SettingsManager&&) = delete;

	bool load(UIState& state);
	bool save(const UIState& state);

private:
	fs::path mConfigPath{};
};