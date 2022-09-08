#include "renderer/vulkan/vulkan_command_pool.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"

VulkanCommandPool::VulkanCommandPool(
    const vk::Device* device,
    const vk::AllocationCallbacks* allocator,
    const vk::Queue* queue,
    const uint32 queue_index
) : _device(device), _allocator(allocator), _queue(queue) {
    vk::CommandPoolCreateInfo command_pool_info{};
    command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    command_pool_info.setQueueFamilyIndex(queue_index);

    try {
        _command_pool = _device->createCommandPool(command_pool_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
}

VulkanCommandPool::~VulkanCommandPool() {
    _device->destroyCommandPool(_command_pool, _allocator);
}

// ////////////// //
// Public methods //
// ////////////// //
vk::CommandBuffer VulkanCommandPool::allocate_command_buffer() {
    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.setCommandPool(_command_pool);
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    alloc_info.setCommandBufferCount(1);

    vk::CommandBuffer command_buffer;
    try {
        command_buffer = _device->allocateCommandBuffers(alloc_info)[0];
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    return command_buffer;
}

std::vector<vk::CommandBuffer> VulkanCommandPool::allocate_command_buffers(uint32 size) {
    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.setCommandPool(_command_pool);
    alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
    alloc_info.setCommandBufferCount(size);

    std::vector<vk::CommandBuffer> command_buffers;
    try {
        command_buffers = _device->allocateCommandBuffers(alloc_info);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    return command_buffers;
}

vk::CommandBuffer VulkanCommandPool::begin_single_time_commands() {
    vk::CommandBufferAllocateInfo allocation_info{};
    allocation_info.setLevel(vk::CommandBufferLevel::ePrimary);
    allocation_info.setCommandBufferCount(1);
    allocation_info.setCommandPool(_command_pool);

    vk::CommandBuffer command_buffer;
    command_buffer = _device->allocateCommandBuffers(allocation_info)[0];

    // Begin recording commands
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info);

    return command_buffer;
}

void VulkanCommandPool::end_single_time_commands(vk::CommandBuffer command_buffer) {
    // Finish recording
    command_buffer.end();

    // Execute command buffer
    vk::SubmitInfo submit_info{};
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&command_buffer);

    _queue->submit(submit_info);
    _queue->waitIdle();

    // Free temp command buffer
    _device->freeCommandBuffers(_command_pool, 1, &command_buffer);
}