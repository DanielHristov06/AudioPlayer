#include "Logger.h"
#include "Utils.h"
#include "imgui.h"
#include "tinyfiledialogs.h"

void Logger::drawWindow() {
	if (mMessage.empty()) return;

	if (mWindowOpen) {
		const char* windowName = "";

		switch (mLevel) {
		case Level::Error: windowName = "Error"; break;
		case Level::Warning: windowName = "Warning"; break;
		}

		ImGui::SetNextWindowSize(ImVec2(350, 120));

		ImGui::Begin(windowName, &mWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		const ImVec2 widgetSpacing = ImVec2(0.0f, 8.0f);

		ImGui::Text(utils::toUtf8(mMessage).c_str());
		const float buttonWidth = ImGui::CalcTextSize("Ok").x + ImGui::GetStyle().FramePadding.x * 2.0f;

		ImGui::Dummy(widgetSpacing);
		ImGui::Separator();
		ImGui::Dummy(widgetSpacing);

		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - buttonWidth) * 0.5f);

		if (ImGui::Button("Ok")) {
			mWindowOpen = false;
			mMessage.clear();
		}

		ImGui::End();
	}
}

void Logger::handleMessage(Level level, const std::string& message) {
	mMessage = message;
	mLevel = level;

	if (level != Level::Error && level != Level::Warning) {
		return;
	}

	if (hasImGuiContext()) mWindowOpen = true;
	else showNativeMessageBox(level, message);
}

void Logger::showNativeMessageBox(Level level, const std::string& message) const {
	const char* title = "Message";
	const char* icon = "info";

	switch (level)
	{
	case Logger::Level::Info:
		title = "Info";
		icon = "info";
		break;
	case Logger::Level::Warning:
		title = "Warning";
		icon = "warning";
		break;
	case Logger::Level::Error:
		title = "Error";
		icon = "error";
		break;
	}

	tinyfd_messageBox(title, message.c_str(), "ok", icon, 1);
}

bool Logger::hasImGuiContext() noexcept {
	return ImGui::GetCurrentContext() != nullptr;
}