#pragma once
#include <string>
#include <format>
#include <utility>

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

		handleMessage(level, message);
	}

	void drawWindow();

private:
	Logger() = default;
	std::string mMessage;

	void handleMessage(Level level, const std::string& message);
	void showNativeMessageBox(Level level, const std::string& message) const;
	bool hasImGuiContext() noexcept;

#if defined(_DEBUG)
	static const char* getPrefix(Level level) noexcept {
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