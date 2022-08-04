#pragma once

#include "platform/platform.hpp"
#if PLATFORM == WINDOWS32 || PLATFORM == LINUX

#include <GLFW/glfw3.h>

class Window : public Platform::Surface {
private:
    GLFWwindow* _window;

    int32 _width;
    int32 _height;
    std::string _name;

public:
    Window(int32 width, int32 height, std::string name);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    vk::SurfaceKHR get_vulkan_surface(vk::Instance& vulkan_instance, vk::AllocationCallbacks* allocator);
    void process_events();
    bool should_close() { return glfwWindowShouldClose(_window); }
};

#endif