#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

#include "logger.hpp"

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

    /// @brief Allocate vulkan command buffer from the pool
    /// @param primary Specify whether the created buffer is a primary buffer
    /// @returns Created and allocated buffer
    vk::CommandBuffer allocate_command_buffer(bool primary = true);
    /// @brief Allocate vulkan command buffers from the pool
    /// @param primary Specify whether created buffers are primary buffers
    /// @returns Created and allocated buffers
    std::vector<vk::CommandBuffer> allocate_command_buffers(uint32 size, bool primary = true);
    /// @brief Returns command buffer to the pool
    /// @param command_buffer Buffer to free
    void free_command_buffer(vk::CommandBuffer command_buffer);
    /// @brief Returns command buffers to the pool
    /// @param command_buffers Buffers to free
    void free_command_buffers(std::vector<vk::CommandBuffer> command_buffers);

    /// @brief Allocate and begin a single use buffer
    /// @returns Created buffer
    vk::CommandBuffer begin_single_time_commands();
    /// @brief End and free a single use buffer
    /// @param command_buffer Buffer to free
    void end_single_time_commands(vk::CommandBuffer command_buffer);
};