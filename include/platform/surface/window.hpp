#ifndef __WINDOW_H__
#define __WINDOW_H__

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

    static void framebuffer_resize_callback(GLFWwindow* window, int32 width, int32 height);

public:
    Window(int32 width, int32 height, std::string name);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    vk::SurfaceKHR get_vulkan_surface(
        const vk::Instance& vulkan_instance,
        const vk::AllocationCallbacks* const allocator
    ) const;

    uint32 get_width_in_pixels();
    uint32 get_height_in_pixels();

    void process_events();
    bool should_close() { return glfwWindowShouldClose(_window); }
};

#endif
#endif // __WINDOW_H__