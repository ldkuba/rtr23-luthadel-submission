#pragma once

#include "vulkan_structures.hpp"

class VulkanDevice {
private:
    vk::AllocationCallbacks* _allocator = nullptr;

public:
    vk::Device handle;
    PhysicalDeviceInfo info;

    QueueFamilyIndices queue_family_indices;
    vk::Queue graphics_queue;
    vk::Queue presentation_queue;
    vk::Queue transfer_queue;
    vk::Queue compute_queue;

    VulkanDevice() {}
    VulkanDevice(
        vk::PhysicalDevice physical_device,
        PhysicalDeviceInfo info,
        QueueFamilyIndices queue_family_indices,
        vk::AllocationCallbacks* allocator
    );
    ~VulkanDevice();

    uint32 find_memory_type(uint32 type_filter, vk::MemoryPropertyFlags properties);
};