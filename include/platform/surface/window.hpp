#pragma once

#include "platform/platform.hpp"

#if PLATFORM == WINDOWS32 || PLATFORM == LINUX

#    include <GLFW/glfw3.h>

/**
 * @brief Implementation of Window, as a surface type.
 */
class Window : public Platform::Surface {
  public:
    Window(int32 width, int32 height, std::string name);
    ~Window();

    // Prevent accidental copying
    Window(Window const&)            = delete;
    Window& operator=(Window const&) = delete;

    Result<vk::SurfaceKHR, RuntimeError> get_vulkan_surface(
        const vk::Instance&                  vulkan_instance,
        const vk::AllocationCallbacks* const allocator
    ) const override;

    uint32 get_width_in_pixels() override;
    uint32 get_height_in_pixels() override;

    void process_events(const float64 delta_time) override;
    bool should_close() override { return glfwWindowShouldClose(_window); }

  private:
    GLFWwindow* _window;

    int32       _width;
    int32       _height;
    std::string _name;

    // Callbacks
    static void framebuffer_resize_callback(
        GLFWwindow* window, int32 width, int32 height
    );
    static void key_callback(
        GLFWwindow* window, int key, int scancode, int action, int mods
    );
};

#endif