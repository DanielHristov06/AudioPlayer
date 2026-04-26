#pragma once
#include <string>
#include <format>

#if defined(_DEBUG)
#include <iostream>
#endif

class Logger {
public:
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(Logger&&) = delete;

	enum class Level { Info, Warning, Error };

	static Logger& get() {
		static Logger instance;
		return instance;
	}

	template<typename... Args>
	void log(Level level, std::format_string<Args...> fmt, Args&&... args) {
		const std::string message = std::format(fmt, std::forward<Args>(args)...);

	#if defined(_DEBUG)
		std::cout << getPrefix(level) << message << '\n';
	#endif

		mMessage = message;
		mLevel = level;

		if (level == Level::Error || level == Level::Warning) {
			mWindowOpen = true;
		}
	}

	void drawWindow();

private:
	Logger() = default;
	std::string mMessage;

#if defined(_DEBUG)
	static const char* getPrefix(Level level) {
		switch (level) {
		case Level::Info: return "[INFO] ";
		case Level::Warning: return "[WARN] ";
		case Level::Error: return "[ERROR] ";
		}
		return "";
	}
#endif

	Level mLevel = Level::Info;
	bool mWindowOpen = false;
};