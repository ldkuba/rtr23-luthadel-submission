#include "platform/platform.hpp"

#if PLATFORM == WINDOWS32

#    include <windows.h>

namespace ENGINE_NAMESPACE {

Platform::Platform() {}

Platform::~Platform() {}

float64 Platform::get_absolute_time() {
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (!QueryPerformanceFrequency(&frequency)) {
        // QueryPerformanceCounter is not supported
        // Handle the error or return a default value
        std::cout << "ERROR :: In function get_absolute_time. "
                     "`QueryPerformanceFrequency` failed."
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!QueryPerformanceCounter(&counter)) {
        // QueryPerformanceCounter failed
        // Handle the error or return a default value
        std::cout << "ERROR :: In function get_absolute_time's "
                     "`QueryPerformanceCounter` failed."
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    return (float64) counter.QuadPart / (float64) frequency.QuadPart;
}

void Platform::sleep(uint64 ms) {
    Sleep(static_cast<DWORD>(ms)); //
}

// /////// //
// Console //
// /////// //

Platform::Console::Console() {}
Platform::Console::~Console() {}

void Platform::Console::write(std::string message, uint32 kind, bool new_line) {
    // Define color attributes for the Windows Console
    const WORD colors[] {
        FOREGROUND_INTENSITY, BACKGROUND_RED | FOREGROUND_INTENSITY,
        FOREGROUND_RED,       FOREGROUND_RED | FOREGROUND_GREEN,
        FOREGROUND_GREEN,     FOREGROUND_BLUE
    };

    // Get the handle to the console output
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get console handle." << std::endl;
        return;
    }

    // Set the text color
    SetConsoleTextAttribute(hConsole, colors[kind]);

    // Write the message to the console
    std::cout << message;

    // Reset text color to default
    SetConsoleTextAttribute(
        hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
    );

    if (new_line) { std::cout << std::endl; }
}

} // namespace ENGINE_NAMESPACE

#endif