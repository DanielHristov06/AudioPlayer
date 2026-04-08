#include "Utils.h"
#include "imgui.h"
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

		if (state.repeatState == UIState::RepeatState::Once && !state.repeatUsed) {
			state.repeatUsed = true;
			player.play(state.currentlyPlayingPath);
			return;
		}
		else if (state.repeatState == UIState::RepeatState::Always) {
			player.play(state.currentlyPlayingPath);
			return;
		}
		else {
			state.repeatUsed = false;
		}

		if (manager.playingMode == LibraryManager::PlayingMode::Queue) {
			const int nextIndex = manager.selectedIndex + 1;
			if (nextIndex < static_cast<int>(manager.mQueue.size())) {
				manager.selectedIndex = nextIndex;
				const fs::path& song = manager.mQueue[nextIndex];
				player.play(song);
				state.currentlyPlayingPath = song;
				return;
			}
			manager.mQueue.clear();
			manager.playingMode = LibraryManager::PlayingMode::None;
		}

		if (manager.playingMode != LibraryManager::PlayingMode::Playlist) {
			manager.mPlayOrderIndex = (manager.mPlayOrderIndex + 1) % static_cast<int>(manager.mPlayOrder.size());
			manager.selectedIndex = manager.getCurrentSongIndex();
			const fs::path& song = manager.mSongs[manager.selectedIndex];
			player.play(song);
			state.currentlyPlayingPath = song;
		}
		else {
			Playlist& currPlaylist = manager.mPlaylists[manager.selectedPlaylist];
			if (currPlaylist.songs.empty()) return;

			currPlaylist.playOrderIndex = (currPlaylist.playOrderIndex + 1) % static_cast<int>(currPlaylist.playOrder.size());
			currPlaylist.selectedIndex = currPlaylist.getCurrentSongIndex();
			player.play(currPlaylist.songs[currPlaylist.selectedIndex]);
			state.currentlyPlayingPath = currPlaylist.songs[currPlaylist.selectedIndex];
		}
	}

	void playPrevSong(UIState& state, LibraryManager& manager, AudioPlayer& player){
		if (manager.mSongs.empty()) return;

		if (manager.playingMode == LibraryManager::PlayingMode::Queue) {
			const int prevIndex = manager.selectedIndex - 1;
			if (prevIndex >= 0) {
				manager.selectedIndex = prevIndex;
				const fs::path& song = manager.mQueue[prevIndex];
				player.play(song);
				state.currentlyPlayingPath = song;
			}
			return;
		}

		if (manager.playingMode != LibraryManager::PlayingMode::Playlist) {
			const int size = static_cast<int>(manager.mPlayOrder.size());
			manager.mPlayOrderIndex = (manager.mPlayOrderIndex - 1 + size) % size;
			manager.selectedIndex = manager.getCurrentSongIndex();

			const fs::path& song = manager.mSongs[manager.selectedIndex];
			player.play(song);
			state.currentlyPlayingPath = song;
		}
		else {
			Playlist& currPlaylist = manager.mPlaylists[manager.selectedPlaylist];
			if (currPlaylist.songs.empty()) return;

			const int size = static_cast<int>(currPlaylist.playOrder.size());
			currPlaylist.playOrderIndex = (currPlaylist.playOrderIndex - 1 + size) % size;
			currPlaylist.selectedIndex = currPlaylist.getCurrentSongIndex();
			player.play(currPlaylist.songs[currPlaylist.selectedIndex]);
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
				std::println("Failed to create directory {} : {}\n", dir.string(), ec.message());
			}
		}
		else if (!fs::is_directory(dir)) {
			std::println("Path '{}' exists but is not a directory.\n", dir.string());
		}
	}

	void openInExplorer(const fs::path& path) {
	#if defined(_WIN32)
		ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	#elif defined(__APPLE__)
		system(("open \"" + path.string() + "\"").c_str());
	#else
		system(("xdg-open \"" + path.string() + "\"").c_str());
	#endif
	}

	void renderSongSelectable(const fs::path& song, LibraryManager::PlayingMode playingMode, int index, int playlistIndex, const std::string& labelSuffix, const char* popupName,
		UIState& state, LibraryManager& manager, AudioPlayer& player)
	{
		const std::string label = toUtf8(song.stem()) + labelSuffix;
		if (ImGui::Selectable(label.c_str(), song == state.currentlyPlayingPath)) {
			if (playlistIndex == -1) {
				manager.selectedIndex = index;
				manager.isPlayingFomPlaylist = false;
				if (manager.selectedPlaylist != -1) {
					manager.mPlaylists[manager.selectedPlaylist].selectedIndex = -1;
					manager.selectedPlaylist = -1;
				}
			}
			else {
				manager.selectedIndex = -1;
				manager.isPlayingFomPlaylist = true;
				manager.selectedPlaylist = playlistIndex;
				manager.mPlaylists[playlistIndex].selectedIndex = index;
			}
			if (song != state.currentlyPlayingPath) {
				player.play(song);
				state.currentlyPlayingPath = song;
				state.repeatUsed = false;
			}
			manager.playingMode = playingMode;
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup(popupName);
			state.popupIndex = index;
			state.popupPlaylistIndex = playlistIndex;
		}
	}

	std::string toUtf8(const fs::path& path) {
		const auto u8 = path.u8string();
		return std::string(reinterpret_cast<const char*>(u8.c_str()), u8.size());
	}
}