#include "Logger.h"
#include "Utils.h"
#include "imgui.h"

void Logger::drawWindow() {
	if (mMessage.empty()) return;

	if (mWindowOpen) {
		ImGui::SetNextWindowSize(ImVec2(350, 120));

		ImGui::Begin("Message", &mWindowOpen, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
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