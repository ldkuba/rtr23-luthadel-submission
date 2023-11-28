#include "renderer/vulkan/vulkan_managed_buffer.hpp"

namespace ENGINE_NAMESPACE {

VulkanManagedBuffer::~VulkanManagedBuffer() { delete _memory_allocator; }

// //////////////////////////////////// //
// VULKAN MANAGED BUFFER PUBLIC METHODS //
// //////////////////////////////////// //

void VulkanManagedBuffer::create(
    const vk::DeviceSize          size,
    const vk::BufferUsageFlags    usage,
    const vk::MemoryPropertyFlags properties,
    const bool                    bind_on_create
) {
    VulkanBuffer::create(size, usage, properties, bind_on_create);
    _memory_allocator = new (MemoryTag::GPUBuffer) GPUFreeListAllocator(
        size, 0, GPUFreeListAllocator::PlacementPolicy::FindFirst
    );
    _memory_allocator->init();
}

void VulkanManagedBuffer::resize(
    const vk::CommandBuffer& command_buffer, const vk::DeviceSize new_size
) {
    Logger::fatal("ERROR :: UNIMPLEMENTED YET"); // TODO: Implement
}

void VulkanManagedBuffer::load_data(
    const void* const    data,
    const vk::DeviceSize offset,
    const vk::DeviceSize size
) const {
    if (_memory_allocator->allocated((void*) offset, size) == false)
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Segmentation fault :D. Cant use unallocated GPU memory."
        );
    VulkanBuffer::load_data(data, offset, size);
}

vk::DeviceSize VulkanManagedBuffer::allocate(
    const uint64 size, const uint64 alignment
) {
    return (vk::DeviceSize) _memory_allocator->allocate(size, alignment);
}
void VulkanManagedBuffer::deallocate(vk::DeviceSize offset) {
    _memory_allocator->free((void*) offset);
}

} // namespace ENGINE_NAMESPACE