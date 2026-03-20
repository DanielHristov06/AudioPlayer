#include "Utils.h"
#include <format>
#include <print>
#include <system_error>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace utils {
	std::string formatTime(const double& seconds) {
		const int s = static_cast<int>(seconds);
		const int min = s / 60;
		const int sec = s % 60;
		return std::format("{}:{:02}", min, sec);
	}

	void playNextSong(UIState& state, LibraryManager& manager, AudioPlayer& player) {
		if (manager.mSongs.empty()) return;

		if (!manager.isPlayingFomPlaylist) {
			if (manager.selectedIndex < 0) {
				manager.selectedIndex = 0;
			}
			else {
				manager.selectedIndex = (manager.selectedIndex + 1) % static_cast<int>(manager.mSongs.size());
			}

			player.play(manager.mSongs[manager.selectedIndex].string());
			state.currentlyPlayingPath = manager.mSongs[manager.selectedIndex];
		}
		else {
			Playlist& currPlaylist = manager.mPlaylists[manager.selectedPlaylist];
			if (currPlaylist.songs.empty()) return;

			if (currPlaylist.selectedIndex < 0) {
				currPlaylist.selectedIndex = 0;
			}
			else {
				currPlaylist.selectedIndex = (currPlaylist.selectedIndex + 1) % static_cast<int>(currPlaylist.songs.size());
			}

			player.play(currPlaylist.songs[currPlaylist.selectedIndex].string());
			state.currentlyPlayingPath = currPlaylist.songs[currPlaylist.selectedIndex];
		}
	}

	void playPrevSong(UIState& state, LibraryManager& manager, AudioPlayer& player){
		if (manager.mSongs.empty()) return;

		if (!manager.isPlayingFomPlaylist) {
			if (manager.selectedIndex < 0) {
				manager.selectedIndex = static_cast<int>(manager.mSongs.size() - 1);
			}
			else {
				const int size = static_cast<int>(manager.mSongs.size());
				manager.selectedIndex = (manager.selectedIndex - 1 + size) % size;
			}

			player.play(manager.mSongs[manager.selectedIndex].string());
			state.currentlyPlayingPath = manager.mSongs[manager.selectedIndex];
		}
		else {
			Playlist& currPlaylist = manager.mPlaylists[manager.selectedPlaylist];
			if (currPlaylist.songs.empty()) return;

			if (currPlaylist.selectedIndex < 0) {
				currPlaylist.selectedIndex = static_cast<int>(currPlaylist.songs.size() - 1);
			}
			else {
				const int size = static_cast<int>(currPlaylist.songs.size());
				currPlaylist.selectedIndex = (currPlaylist.selectedIndex - 1 + size) % size;
			}

			player.play(currPlaylist.songs[currPlaylist.selectedIndex].string());
			state.currentlyPlayingPath = currPlaylist.songs[currPlaylist.selectedIndex];
		}
	}

	fs::path getBasePath() {
	#if defined(_WIN32)
		const char* appData = std::getenv("APPDATA");
		return appData ? fs::path(appData) : fs::current_path();
	#elif defined(__APPLE__)
		const char* home = std::getenv("HOME");
		return home ? fs::path(home) / "Library" / "Application Support" : fs::current_path();
	#else
		const char* xdgData = std::getenv("XDG_DATA_HOME");
		if (xdgData) return fs::path(xdgData);

		const char* home = std::getenv("HOME");
		return home ? fs::path(home) / ".local" / "share" : fs::current_path();
	#endif
	}

	void createDirectory(const fs::path& dir) {
		if (!fs::exists(dir)) {
			std::error_code ec;
			fs::create_directories(dir, ec);

			if (ec) {
				std::println("Failed to create directory {} : {}", dir.string(), ec.message());
			}
		}
		else if (!fs::is_directory(dir)) {
			std::println("Path '{}' exists but is not a directory.\n", dir.string());
		}
	}

	void openInExplorer(const fs::path& path) {
	#if defined(_WIN32)
		ShellExecuteA(nullptr, "open", path.string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	#elif defined(__APPLE__)
		system(("open \"" + path.string() + "\"").c_str());
	#else
		system(("xdg-open \"" + path.string() + "\"").c_str());
	#endif
	}

#if defined(_WIN32)
	std::wstring toWide(const std::string& str) {
		if (str.empty()) return {};
		const int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
		std::wstring result(size - 1, 0);
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
		return result;
	}
#endif
}