#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "platform/platform.hpp"
#include "renderer/renderer_backend.hpp"
#include "vulkan_device.hpp"

class VulkanBackend : public RendererBackend {
private:
    // TODO: Custom allocator
    vk::AllocationCallbacks* _allocator = nullptr;

    vk::Instance _vulkan_instance;
    vk::DebugUtilsMessengerEXT _debug_messenger;
    vk::SurfaceKHR _vulkan_surface;

    VulkanDevice* _device;

#ifdef NDEBUG
    const bool _enable_validation_layers = false;
#else
    const bool _enable_validation_layers = true;
#endif
    const std::vector<const char*> _validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    void create_vulkan_instance();
    void setup_debug_messenger();

    bool all_validation_layers_are_available();
    vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();

public:
    VulkanBackend(Platform::Surface* surface);
    ~VulkanBackend();

    void resized(uint32 width, uint32 height);
    bool begin_frame(float32 delta_time);
    bool end_frame(float32 delta_time);
};