#pragma once

#include "vulkan_image.hpp"

class VulkanBuffer {
private:

    VulkanDevice* _device;
    vk::AllocationCallbacks* _allocator;

    vk::BufferUsageFlags _usage;
    vk::MemoryPropertyFlags _properties;

public:
    vk::Buffer handle;
    vk::DeviceMemory memory;
    vk::DeviceSize size;

    VulkanBuffer(VulkanDevice* device, vk::AllocationCallbacks* allocator) : _device(device), _allocator(allocator) {}
    ~VulkanBuffer();


    void create(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        bool bind_on_create = true
    );

    void bind(vk::DeviceSize offset);

    void resize(
        VulkanCommandPool* command_pool,
        vk::DeviceSize new_size
    );

    void load_data(
        const void* data,
        vk::DeviceSize offset,
        vk::DeviceSize size,
        vk::MemoryMapFlags flags = {}
    );

    void copy_data_to_buffer(
        VulkanCommandPool* command_pool,
        vk::Buffer& buffer,
        vk::DeviceSize source_offset,
        vk::DeviceSize destination_offset,
        vk::DeviceSize size
    );

    void copy_data_to_image(
        VulkanCommandPool* command_pool,
        VulkanImage& image
    );
};