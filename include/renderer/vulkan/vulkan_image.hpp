#pragma once

#include "vulkan_command_pool.hpp"
#include "vulkan_device.hpp"

class VulkanImage {
private:
    VulkanDevice* _device;
    vk::AllocationCallbacks* _allocator;

    bool _has_view = false;

    void create_view(
        uint32 mip_levels,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags
    );

public:
    vk::Image handle;
    vk::DeviceMemory memory;
    vk::ImageView view;

    uint32 width;
    uint32 height;

    VulkanImage(VulkanDevice* device, vk::AllocationCallbacks* allocator) : _device(device), _allocator(allocator) {}
    ~VulkanImage();

    void create(
        uint32 width,
        uint32 height,
        uint32 mip_levels,
        vk::SampleCountFlagBits number_of_samples,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties
    );

    void create(
        uint32 width,
        uint32 height,
        uint32 mip_levels,
        vk::SampleCountFlagBits number_of_samples,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::ImageAspectFlags aspect_flags
    );

    void create(
        vk::Image image,
        uint32 mip_levels,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags
    );

    void transition_image_layout(
        VulkanCommandPool* command_pool,
        vk::Format format,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        uint32 mip_levels
    );

    static vk::ImageView get_view_from_image(
        vk::Format format,
        vk::ImageAspectFlags aspect_flags,
        const vk::Image& image,
        const vk::Device& device,
        const vk::AllocationCallbacks* allocator
    );
};
