#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "math_libs.hpp"

#include "systems/memory/memory_system.hpp"
#include "app_temp.hpp"

// TODO: APPLICATION / ENGINE SPLIT
// TODO: UNIT TESTING

int main(int, char**) {
    TestApplication* app = new (MemoryTag::Application) TestApplication {};

#ifdef NDEBUG
    Logger::debug("MODE is RELEASE");
#else
    Logger::debug("MODE is DEBUG");
#endif

    app->run();

    delete app;
    MemorySystem::reset_memory(MemoryTag::Application);

    return EXIT_SUCCESS;
}