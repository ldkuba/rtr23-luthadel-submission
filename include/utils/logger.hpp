#pragma once

#include <string>

#include "platform.hpp"

class Logger {
private:
    static const bool warning_enabled;
    static const bool info_enabled;
    static const bool debug_enabled;

public:
    Logger(/* args */);
    ~Logger();

    static void fatal(std::string message);
    static void error(std::string message);
    static void warning(std::string message);
    static void log(std::string message);
    static void debug(std::string message);
};
