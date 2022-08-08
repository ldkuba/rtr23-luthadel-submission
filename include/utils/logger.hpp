#pragma once

#include "defines.hpp"
#include <string>

#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_VERBOSE_ENABLED 0

#include "platform/platform.hpp"

class Logger {
private:

    template<typename T>
    static void logger_output_one(uint32 kind, T message) {
        Platform::Console::write(std::to_string(message), kind, false);
    }

    template<typename... Args>
    static void logger_output(std::string initial, uint32 kind, Args... message) {
        Platform::Console::write(initial + " :: ", kind, false);
        (logger_output_one(kind, message), ...);
        Platform::Console::write("");
    }


public:
    Logger();
    ~Logger();

    /**
     * @brief Logs given fatal error message.
     *
     * Parameter list automaticaly converted to std::string via std::to_string if possible,
     * otherwise throws appropriate error.
     * Parameter list is also automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void fatal(Args... message) {
        logger_output(std::string("FATAL ERROR"), 1, message...);
    }
    /**
     * @brief Logs given errors message.
     *
     * Parameter list automaticaly converted to std::string via std::to_string if possible,
     * otherwise throws appropriate error.
     * Argument list is also automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void error(Args... message) {
        logger_output(std::string("ERR"), 2, message...);
    }
    /**
     * @brief Logs given warning message if LOG_WARNING_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string if possible,
     * otherwise throws appropriate error.
     * Argument list is also automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void warning(Args... message) {
#if LOG_WARNING_ENABLED
        logger_output(std::string("WAR"), 3, message...);
#endif
    }

    /**
     * @brief Logs given info message if LOG_INFO_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string if possible,
     * otherwise throws appropriate error.
     * Argument list is also automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void log(Args... message) {
#if LOG_INFO_ENABLED
        logger_output(std::string("INF"), 4, message...);
#endif
    }
    /**
     * @brief Logs given debug message if LOG_DEBUG_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string if possible,
     * otherwise throws appropriate error.
     * Argument list is also automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void debug(Args... message) {
#if LOG_DEBUG_ENABLED
        logger_output(std::string("DEB"), 5, message...);
#endif
    }
    /**
     * @brief Logs given verbose message if LOG_VERBOSE_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string if possible,
     * otherwise throws appropriate error.
     * Argument list is also automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void verbose(Args... message) {
#if LOG_VERBOSE_ENABLED
        logger_output(std::string("VER"), 0, message...);
#endif
    }
};

template<>
void Logger::logger_output_one<const char*>(uint32 kind, const char* message);
template<>
void Logger::logger_output_one<std::string>(uint32 kind, std::string message);