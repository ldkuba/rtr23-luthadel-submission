#pragma once

#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "utils/defines.hpp"

class Window {
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

    bool should_close() { return glfwWindowShouldClose(_window); }
};
