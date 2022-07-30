#pragma once

#include <string>

#include "platform/platform.hpp"

#define LOG_WARNING_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1

class Logger {
public:
    Logger();
    ~Logger();

    static void fatal(std::string message);
    static void error(std::string message);
    static void warning(std::string message);
    static void log(std::string message);
    static void debug(std::string message);
};
