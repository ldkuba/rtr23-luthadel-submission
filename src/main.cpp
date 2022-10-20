#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "memory_system.hpp"
#include "systems/app_temp.hpp"

// TODO: APPLICATION / ENGINE SPLIT
// TODO: UNIT TESTING

int main(int, char**) {

    TestApplication app {};

#ifdef NDEBUG
    Logger::debug("MODE is RELEASE");
#else
    Logger::debug("MODE is DEBUG");
#endif

    app.run();

    return EXIT_SUCCESS;
}