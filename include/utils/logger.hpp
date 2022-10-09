#pragma once

#include "defines.hpp"
#include "string.hpp"

#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_VERBOSE_ENABLED 1

#include "platform/platform.hpp"

class Logger {
  public:

    Logger() {}
    ~Logger() {}

    /**
     * @brief Logs given fatal error message.
     *
     * Parameter list automaticaly converted to std::string via std::to_string
     * if possible, otherwise throws appropriate error. Parameter list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void fatal(Args... message) {
        auto full_message =
            String::build(std::string("FATAL ERROR"), " :: ", message...);
        Platform::Console::write(full_message, 1, true);
        exit(EXIT_FAILURE);
    }
    /**
     * @brief Logs given errors message.
     *
     * Parameter list automaticaly converted to std::string via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void error(Args... message) {
        auto full_message =
            String::build(std::string("ERR"), " :: ", message...);
        Platform::Console::write(full_message, 2, true);
    }
    /**
     * @brief Logs given warning message if LOG_WARNING_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void warning(Args... message) {
#if LOG_WARNING_ENABLED
        auto full_message =
            String::build(std::string("WAR"), " :: ", message...);
        Platform::Console::write(full_message, 3, true);
#endif
    }

    /**
     * @brief Logs given info message if LOG_INFO_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void log(Args... message) {
#if LOG_INFO_ENABLED
        auto full_message =
            String::build(std::string("INF"), " :: ", message...);
        Platform::Console::write(full_message, 4, true);
#endif
    }
    /**
     * @brief Logs given debug message if LOG_DEBUG_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void debug(Args... message) {
#if LOG_DEBUG_ENABLED
        auto full_message =
            String::build(std::string("DEB"), " :: ", message...);
        Platform::Console::write(full_message, 5, true);
#endif
    }
    /**
     * @brief Logs given trace message if LOG_VERBOSE_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to std::string via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void trace(Args... message) {
#if LOG_VERBOSE_ENABLED
        auto full_message =
            String::build(std::string("VER"), " :: ", message...);
        Platform::Console::write(full_message, 0, true);
#endif
    }
};