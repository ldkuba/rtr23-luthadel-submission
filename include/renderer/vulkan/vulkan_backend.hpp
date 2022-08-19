#pragma once

#include "renderer/renderer_backend.hpp"
#include "vulkan_device.hpp"

class VulkanBackend : public RendererBackend {
private:
    // TODO: Custom allocator
    vk::AllocationCallbacks* _allocator = nullptr;

    vk::Instance _vulkan_instance;
    vk::DebugUtilsMessengerEXT _debug_messenger;

    VulkanDevice* _device;

    const std::vector<const char*> _validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

    void create_vulkan_instance();
    void setup_debug_messenger();

    void create_graphics_pipeline();

    bool all_validation_layers_are_available();
    vk::DebugUtilsMessengerCreateInfoEXT debug_messenger_create_info();

public:
    VulkanBackend(Platform::Surface* surface);
    ~VulkanBackend();

    void resized(uint32 width, uint32 height);
    bool begin_frame(float32 delta_time);
    bool end_frame(float32 delta_time);

    void wait_for_shutdown() {
        _device->wait_idle();
    }
};