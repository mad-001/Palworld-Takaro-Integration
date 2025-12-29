#pragma once

#include <string>
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/fmt.h>

namespace TakaroPalworld {

class Logger {
public:
    static Logger& GetInstance();

    void Initialize(const std::string& logDir, size_t maxSize, size_t maxFiles);
    void Shutdown();

    void SetLevel(const std::string& level);

    template<typename... Args>
    void Debug(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->debug(fmt::runtime(format), std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void Info(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->info(fmt::runtime(format), std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void Warning(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->warn(fmt::runtime(format), std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void Error(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->error(fmt::runtime(format), std::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    void Critical(const std::string& format, Args&&... args) {
        if (logger_) {
            logger_->critical(fmt::runtime(format), std::forward<Args>(args)...);
        }
    }

    std::shared_ptr<spdlog::logger> GetLogger() { return logger_; }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::shared_ptr<spdlog::logger> logger_;
};

// Convenience macros
#define LOG_DEBUG(...) TakaroPalworld::Logger::GetInstance().Debug(__VA_ARGS__)
#define LOG_INFO(...) TakaroPalworld::Logger::GetInstance().Info(__VA_ARGS__)
#define LOG_WARNING(...) TakaroPalworld::Logger::GetInstance().Warning(__VA_ARGS__)
#define LOG_ERROR(...) TakaroPalworld::Logger::GetInstance().Error(__VA_ARGS__)
#define LOG_CRITICAL(...) TakaroPalworld::Logger::GetInstance().Critical(__VA_ARGS__)

} // namespace TakaroPalworld
