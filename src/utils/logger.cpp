#include "logger.hpp"


Logger::Logger(/* args */) {}

Logger::~Logger() {}


template<>
void Logger::logger_output_one<const char*>(uint32 kind, const char* message) {
    Platform::Console::write(message, kind, false);
}

template<>
void Logger::logger_output_one<std::string>(uint32 kind, std::string message) {
    Platform::Console::write(message, kind, false);
}