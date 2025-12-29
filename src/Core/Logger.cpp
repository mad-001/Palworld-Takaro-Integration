#include "TakaroPalworld/Core/Logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <filesystem>
#include <fstream>
#include <ctime>
#include <windows.h>

namespace TakaroPalworld {

// Helper function to write error to fallback file
static void WriteFallbackError(const std::string& message) {
    try {
        HANDLE hFile = CreateFileA(
            "Takaro-Logger-Error.txt",
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            std::time_t now = std::time(nullptr);
            char timeStr[100];
            std::strftime(timeStr, sizeof(timeStr), "[%Y-%m-%d %H:%M:%S] ", std::localtime(&now));

            std::string fullMsg = std::string(timeStr) + message + "\n";
            DWORD written;
            WriteFile(hFile, fullMsg.c_str(), fullMsg.length(), &written, NULL);
            CloseHandle(hFile);
        }
    } catch (...) {
        // Nothing we can do
    }
}

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

void Logger::Initialize(const std::string& logDir, size_t maxSize, size_t maxFiles) {
    try {
        WriteFallbackError("Logger::Initialize() called with logDir: " + logDir);

        // Validate parameters
        if (maxSize == 0) {
            throw std::invalid_argument("rotating sink constructor: max_size arg cannot be zero");
        }
        if (maxFiles > 200000) {
            throw std::invalid_argument("rotating sink constructor: max_files arg cannot exceed 200000");
        }

        // Create log directory if it doesn't exist
        WriteFallbackError("Attempting to create directory: " + logDir);
        std::error_code ec;
        std::filesystem::create_directories(logDir, ec);

        if (ec) {
            std::string errMsg = "Failed to create log directory: " + ec.message();
            WriteFallbackError(errMsg);
            throw std::runtime_error(errMsg);
        }

        WriteFallbackError("Directory created successfully");

        // Create sinks
        WriteFallbackError("Creating console sink...");
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        WriteFallbackError("Creating rotating file sink...");
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            logDir + "/Takaro-Palworld-Integration.log", maxSize, maxFiles);
        file_sink->set_level(spdlog::level::trace);

        // Create logger with both sinks
        WriteFallbackError("Creating logger with sinks...");
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        logger_ = std::make_shared<spdlog::logger>("Takaro-Palworld-Integration", sinks.begin(), sinks.end());

        // Set pattern
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
        logger_->set_level(spdlog::level::info);

        // Register logger
        WriteFallbackError("Registering logger...");
        spdlog::register_logger(logger_);

        WriteFallbackError("Logger initialized successfully!");
        Info("Logger initialized successfully");
    } catch (const std::exception& e) {
        std::string errMsg = "EXCEPTION in Logger::Initialize: " + std::string(e.what());
        WriteFallbackError(errMsg);

        if (logger_) {
            Error("Failed to initialize logger: {}", e.what());
        }
        throw;
    }
}

void Logger::Shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::drop("Takaro-Palworld-Integration");
        logger_.reset();
    }
}

void Logger::SetLevel(const std::string& level) {
    if (!logger_) return;

    if (level == "trace") {
        logger_->set_level(spdlog::level::trace);
    } else if (level == "debug") {
        logger_->set_level(spdlog::level::debug);
    } else if (level == "info") {
        logger_->set_level(spdlog::level::info);
    } else if (level == "warn" || level == "warning") {
        logger_->set_level(spdlog::level::warn);
    } else if (level == "error") {
        logger_->set_level(spdlog::level::err);
    } else if (level == "critical") {
        logger_->set_level(spdlog::level::critical);
    } else {
        Warning("Unknown log level: {}, defaulting to info", level);
        logger_->set_level(spdlog::level::info);
    }
}

} // namespace TakaroPalworld
