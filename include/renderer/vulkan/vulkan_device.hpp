#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>

class VulkanDevice {
private:
    vk::Instance* _vulkan_instance;

    vk::PhysicalDevice _physical_device = VK_NULL_HANDLE;
    vk::Device _logical_device;

    vk::Queue _graphics_queue;
    vk::Queue _presentation_queue;

    void pick_physical_device(const vk::SurfaceKHR& surface);
    void create_logical_device(const vk::SurfaceKHR& surface);

public:
    VulkanDevice(vk::Instance* instance, const vk::SurfaceKHR& surface);
    ~VulkanDevice();
};