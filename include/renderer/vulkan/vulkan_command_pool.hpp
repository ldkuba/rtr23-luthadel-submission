#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

#include "defines.hpp"

class VulkanCommandPool {
private:
    const vk::Device* _device;
    const vk::AllocationCallbacks* _allocator;
    const vk::Queue* _queue;

    vk::CommandPool _command_pool;

public:
    VulkanCommandPool(
        const vk::Device* device,
        const vk::AllocationCallbacks* allocator,
        const vk::Queue* queue,
        const uint32 queue_index
    );
    ~VulkanCommandPool();

    vk::CommandBuffer allocate_command_buffer();
    std::vector<vk::CommandBuffer> allocate_command_buffers(uint32 size);

    vk::CommandBuffer begin_single_time_commands();
    void end_single_time_commands(vk::CommandBuffer command_buffer);
};