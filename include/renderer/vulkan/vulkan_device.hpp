#pragma once

#include <vulkan/vulkan.hpp>

#include "defines.hpp"

class VulkanDevice {
private:
    vk::Instance* _vulkan_instance;

    vk::PhysicalDevice _physical_device = VK_NULL_HANDLE;
    vk::Device _logical_device;

    vk::Queue _graphics_queue;

    void pick_physical_device();
    void create_logical_device();

public:
    VulkanDevice(vk::Instance* instance);
    ~VulkanDevice();
};