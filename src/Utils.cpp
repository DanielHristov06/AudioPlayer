#include "Utils.h"
#include <format>

std::string formatTime(double& seconds) {
	int s = static_cast<int>(seconds);
	int min = s / 60;
	int sec = s % 60;
	return std::format("{}:{:02}", min, sec);
}