#pragma once
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>
class Log
{
public:
	static Log* getInstance() {
		static Log m_instance;
		return &m_instance;
	}
	// function
private:
	std::shared_ptr<spdlog::logger>	m_loggerConsole;
	Log() {
		m_loggerConsole = spdlog::stdout_color_mt("console");
	}
public:
	template <typename... Args>
	void info(const char* message, const Args&... args) {
		m_loggerConsole->info(message, args...);
	}
	template <typename... Args>
	void error(const char* message, const Args&... args) {
		m_loggerConsole->error(message, args...);
	}
};

#define LOG_INFO(message, ...) Log::getInstance()->info(message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) Log::getInstance()->error(message, ##__VA_ARGS__)