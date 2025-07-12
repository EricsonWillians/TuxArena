#ifndef TUXARENA_LOG_H
#define TUXARENA_LOG_H

#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>

// Define this to 0 to disable INFO logs
#define LOG_INFO_ENABLED 1
#define LOG_WARNING_ENABLED 1
#define LOG_ERROR_ENABLED 1

namespace TuxArena {

class Log {
public:
    enum class Level {
        INFO,
        WARNING,
        ERROR
    };

    static void Info(const std::string& message) {
#if LOG_INFO_ENABLED
        (void)message; // Suppress unused parameter warning
        log(Level::INFO, message);
#endif
    }

    static void Warning(const std::string& message) {
#if LOG_WARNING_ENABLED
        (void)message; // Suppress unused parameter warning
        log(Level::WARNING, message);
#endif
    }

    static void Error(const std::string& message) {
#if LOG_ERROR_ENABLED
        (void)message; // Suppress unused parameter warning
        log(Level::ERROR, message);
#endif
    }

private:
    static void log(Level level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::cout << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S") << "] ";

        switch (level) {
            case Level::INFO:
                std::cout << "[INFO] ";
                break;
            case Level::WARNING:
                std::cout << "[WARNING] ";
                break;
            case Level::ERROR:
                std::cout << "[ERROR] ";
                break;
        }
        std::cout << message << std::endl;
    }
};

} // namespace TuxArena

#endif // TUXARENA_LOG_H
