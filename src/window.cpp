#include "window.hpp"

Window::Window(int32 width, int32 height, std::string name) : _width(width), _height(height), _name(name) {
    // initialize GLFW with parameters
    glfwInit();                                     // Initialize GLFW
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);   // We dont want a OpenGL context
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);     // Window resize disabled TODO: enable

    // Create window :: width, height, window name, monitor, share (OpenGL only)
    _window = glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);
}

Window::~Window() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}