#pragma once

#include "string.hpp"
#include "platform/platform.hpp"

namespace ENGINE_NAMESPACE {

class Logger {
  private:
    Logger() {}
    ~Logger() {}

    const static bool log_warning;
    const static bool log_info;
    const static bool log_debug;
    const static bool log_verbose;

  public:
    /**
     * @brief Logs given fatal error message.
     *
     * Parameter list automaticaly converted to String via std::to_string
     * if possible, otherwise throws appropriate error. Parameter list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void fatal(const Args&... message) {
        auto full_message =
            String::build(String("FATAL ERROR"), " :: ", message...);
        Platform::Console::write(full_message, 1, true);
        exit(EXIT_FAILURE);
    }
    /**
     * @brief Logs given errors message.
     *
     * Parameter list automaticaly converted to String via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void error(const Args&... message) {
        auto full_message = String::build(String("ERR"), " :: ", message...);
        Platform::Console::write(full_message, 2, true);
    }
    /**
     * @brief Logs given warning message if LOG_WARNING_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to String via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void warning(const Args&... message) {
        if (!log_warning) return;
        auto full_message = String::build(String("WAR"), " :: ", message...);
        Platform::Console::write(full_message, 3, true);
    }

    /**
     * @brief Logs given info message if LOG_INFO_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to String via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void log(const Args&... message) {
        if (!log_info) return;
        auto full_message = String::build(String("INF"), " :: ", message...);
        Platform::Console::write(full_message, 4, true);
    }
    /**
     * @brief Logs given debug message if LOG_DEBUG_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to String via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void debug(const Args&... message) {
        if (!log_debug) return;
        auto full_message = String::build(String("DEB"), " :: ", message...);
        Platform::Console::write(full_message, 5, true);
    }
    /**
     * @brief Logs given trace message if LOG_VERBOSE_ENABLED is set to one.
     *
     * Parameter list automaticaly converted to String via std::to_string
     * if possible, otherwise throws appropriate error. Argument list is also
     * automaticaly concatenated, ending with a new line.
     */
    template<typename... Args>
    static void trace(const Args&... message) {
        if (!log_verbose) return;
        auto full_message = String::build(String("VER"), " :: ", message...);
        Platform::Console::write(full_message, 0, true);
    }

    // Classes for error data auto-reporting
    class __REPORT_FATAL__ {
      public:
        __REPORT_FATAL__(
            const String caller, const String file, const uint32 line
        )
            : _caller(caller), _file(file), _line(line) {}

        template<typename... Args>
        void operator()(Args... message) {
            fatal(
                message...,
                "\n :: File \"",
                _file,
                "\", line ",
                _line,
                ". Function ",
                _caller,
                "()."
            );
        }

      private:
        const String _caller;
        const String _file;
        const uint32 _line;
    };
    class __REPORT_ERROR__ {
      public:
        __REPORT_ERROR__(
            const String caller, const String file, const uint32 line
        )
            : _caller(caller), _file(file), _line(line) {}

        template<typename... Args>
        void operator()(Args... message) {
            error(
                message...,
                "\n :: File \"",
                _file,
                "\", line ",
                _line,
                ". Function ",
                _caller,
                "."
            );
        }

      private:
        const String _caller;
        const String _file;
        const uint32 _line;
    };
};

// #undef error
// #define error __REPORT_ERROR__(__PRETTY_FUNCTION__, __FILE__, __LINE__)

#undef fatal
#define fatal __REPORT_FATAL__(__PRETTY_FUNCTION__, __FILE__, __LINE__)

#define LOG_LOCATION                                                           \
    "\n :: File \"", __FILE__, "\", line ", __LINE__, ". Function ",           \
        __PRETTY_FUNCTION__, "."

} // namespace ENGINE_NAMESPACE
