#include "platform/platform.hpp"
#if PLATFORM == WINDOWS32 || PLATFORM == LINUX

#    include "platform/surface/window.hpp"

Platform::Surface* Platform::Surface::get_instance(
    uint32 width, uint32 height, std::string name
) {
    return new Window(width, height, name);
}

const std::vector<const char*> Platform::get_required_vulkan_extensions() {
    uint32_t count;
    auto     extensions = glfwGetRequiredInstanceExtensions(&count);
    return std::vector(extensions, extensions + count);
}

Window::Window(int32 width, int32 height, std::string name)
    : _width(width), _height(height), _name(name) {
    // initialize GLFW with parameters
    glfwInit(); // Initialize GLFW
    glfwWindowHint(
        GLFW_CLIENT_API, GLFW_NO_API
    ); // We dont want a OpenGL context

    // Create window :: width, height, window name, monitor, share (OpenGL only)
    _window =
        glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);

    // Setup resize callback
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebuffer_resize_callback);
}

Window::~Window() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

void Window::framebuffer_resize_callback(
    GLFWwindow* window, int32 width, int32 height
) {
    auto surface = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    surface->resize_event(width, height);
}

vk::SurfaceKHR Window::get_vulkan_surface(
    const vk::Instance&                  vulkan_instance,
    const vk::AllocationCallbacks* const allocator
) const {
    vk::SurfaceKHR vulkan_surface;
    vk::Result     result = static_cast<vk::Result>(glfwCreateWindowSurface(
        vulkan_instance,
        _window,
        (VkAllocationCallbacks*) allocator,
        (VkSurfaceKHR*) &vulkan_surface
    ));
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create window surface.");
    return vulkan_surface;
}

uint32 Window::get_width_in_pixels() {
    int32 width;
    glfwGetFramebufferSize(_window, &width, nullptr);
    return static_cast<uint32>(width);
}
uint32 Window::get_height_in_pixels() {
    int32 height;
    glfwGetFramebufferSize(_window, nullptr, &height);
    return static_cast<uint32>(height);
}

void Window::process_events() {
    glfwPollEvents();
    if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        exit(EXIT_SUCCESS);
    }
}

#endif