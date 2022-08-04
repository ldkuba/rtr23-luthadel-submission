#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "test_app.hpp"

int main(int, char**) {

    TestApplication app{};

#ifdef NDEBUG
    Logger::debug("MODE is RELEASE");
#else
    Logger::debug("MODE is DEBUG");
#endif

    try {
        app.run();
    } catch (const std::exception& e) {
        Logger::fatal(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}