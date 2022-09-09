#include "renderer/vulkan/vulkan_command_pool.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"

VulkanCommandPool::VulkanCommandPool(
    const vk::Device* device,
    const vk::AllocationCallbacks* allocator,
    const vk::Queue* queue,
    const uint32 queue_index
) : _device(device), _allocator(allocator), _queue(queue) {
    // Create vulkan command pool
    vk::CommandPoolCreateInfo command_pool_info{};
    // Allows for separate reset of buffers from the pool
    command_pool_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    // The queue to which pull command buffers will be submitted to
    command_pool_info.setQueueFamilyIndex(queue_index);

    try {
        _command_pool = _device->createCommandPool(command_pool_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
}

VulkanCommandPool::~VulkanCommandPool() {
    _device->destroyCommandPool(_command_pool, _allocator);
}

// /////////////// //
// Private methods //
// /////////////// //



// ////////////// //
// Public methods //
// ////////////// //
vk::CommandBuffer VulkanCommandPool::allocate_command_buffer(bool primary) {
    return allocate_command_buffers(1, primary)[0];
}

std::vector<vk::CommandBuffer> VulkanCommandPool::allocate_command_buffers(uint32 size, bool primary) {
    // Allocate command buffers from the pool
    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.setCommandPool(_command_pool);   // Pool reference
    alloc_info.setCommandBufferCount(size);     // Number of buffers to be created
    // Command buffer level. Secondary command buffers cannot be submitted directly to the device queue,
    // Instead they are used by primary command buffers.
    alloc_info.setLevel(primary ? vk::CommandBufferLevel::ePrimary : vk::CommandBufferLevel::eSecondary);

    std::vector<vk::CommandBuffer> command_buffers;
    try {
        command_buffers = _device->allocateCommandBuffers(alloc_info);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    return command_buffers;
}

void VulkanCommandPool::free_command_buffer(vk::CommandBuffer command_buffer) {
    _device->freeCommandBuffers(_command_pool, 1, &command_buffer);
}

void VulkanCommandPool::free_command_buffers(std::vector<vk::CommandBuffer> command_buffers) {
    _device->freeCommandBuffers(_command_pool, command_buffers);
}

vk::CommandBuffer VulkanCommandPool::begin_single_time_commands() {
    vk::CommandBuffer command_buffer = allocate_command_buffer();

    // Begin recording commands (indicating the single use property)
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
    free_command_buffer(command_buffer);
}