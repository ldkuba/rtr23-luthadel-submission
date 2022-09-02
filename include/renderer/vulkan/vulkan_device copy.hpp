/*
#pragma once

#include "vulkan_structures.hpp"

class VulkanDevice {
private:
    vk::SurfaceKHR _vulkan_surface;
    vk::AllocationCallbacks* _allocator = nullptr;

    vk::PhysicalDevice pick_physical_device(const vk::Instance& instance);
    void create_logical_device(vk::PhysicalDevice physical_device);

    PhysicalDeviceInfo get_physical_device_info(vk::PhysicalDevice physical_device);
    int32 rate_device_suitability(const vk::PhysicalDevice& device);
    QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device);
    SwapchainSupportDetails query_swapchain_support_details(const vk::PhysicalDevice& device);

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
        const vk::Instance& instance,
        const vk::SurfaceKHR& surface,
        vk::AllocationCallbacks* allocator
    );
    ~VulkanDevice();

    uint32 find_memory_type(uint32 type_filter, vk::MemoryPropertyFlags properties);
};
*/