#pragma once

#include "string.hpp"
#include "platform/platform.hpp"

namespace ENGINE_NAMESPACE {

#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_VERBOSE_ENABLED 1

class Logger {
  private:
    Logger() {}
    ~Logger() {}

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
#if LOG_WARNING_ENABLED
        auto full_message = String::build(String("WAR"), " :: ", message...);
        Platform::Console::write(full_message, 3, true);
#endif
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
#if LOG_INFO_ENABLED
        auto full_message = String::build(String("INF"), " :: ", message...);
        Platform::Console::write(full_message, 4, true);
#endif
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
#if LOG_DEBUG_ENABLED
        auto full_message = String::build(String("DEB"), " :: ", message...);
        Platform::Console::write(full_message, 5, true);
#endif
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
#if LOG_VERBOSE_ENABLED
        auto full_message = String::build(String("VER"), " :: ", message...);
        Platform::Console::write(full_message, 0, true);
#endif
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

#undef fatal
#define fatal __REPORT_FATAL__(__PRETTY_FUNCTION__, __FILE__, __LINE__)
#define LOG_LOCATION                                                           \
    "\n :: File \"", __FILE__, "\", line ", __LINE__, ". Function ",           \
        __PRETTY_FUNCTION__, "."

    // #undef error
    // #define error __REPORT_ERROR__(__func__, __FILE__, __LINE__)
};

} // namespace ENGINE_NAMESPACE

#undef LOG_WARNING_ENABLED
#undef LOG_INFO_ENABLED
#undef LOG_DEBUG_ENABLED
#undef LOG_VERBOSE_ENABLED