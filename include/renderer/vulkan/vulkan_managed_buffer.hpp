#pragma once

#include "vulkan_buffer.hpp"
#include "memory_allocators/gpu_free_list_allocator.hpp"

class VulkanManagedBuffer : public VulkanBuffer {
  public:
    using VulkanBuffer::VulkanBuffer;

    VulkanManagedBuffer(
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator
    )
        : VulkanBuffer(device, allocator) {}
    ~VulkanManagedBuffer();

    /// @brief Create and allocate on device memory for this buffer
    /// @param size Required buffer size
    /// @param usage Buffer usage flags
    /// @param properties Required properties for on device memory
    /// @param bind_on_create Binds created buffer to the allocated memory if
    /// true, default = true
    void create(
        const vk::DeviceSize          size,
        const vk::BufferUsageFlags    usage,
        const vk::MemoryPropertyFlags properties,
        const bool                    bind_on_create = true
    );

    /// @brief Resize buffer
    /// @param command_buffer Command buffer to witch the resize command will be
    /// submitted
    /// @param new_size New buffer size in bytes
    void resize(
        const vk::CommandBuffer& command_buffer, const vk::DeviceSize new_size
    );

    /// @brief Upload data to buffer. If the region denoted by the offset and
    /// size isn't fully allocated raises segmentation error.
    /// @param data Byte array to be uploaded
    /// @param offset Data offset
    /// @param size Total data size in bytes
    void load_data(
        const void* const    data,
        const vk::DeviceSize offset,
        const vk::DeviceSize size
    ) const;

    /// @brief Allocates buffer memory.
    /// @param size Requested allocation size
    /// @returns In buffer offset at which the allocated region starts.
    vk::DeviceSize allocate(uint64 size);

    /// @brief Deallocates part of the buffer memory.
    /// @param offset In buffer offset at which the allocated region starts.
    void deallocate(vk::DeviceSize offset);

  private:
    GPUFreeListAllocator* _memory_allocator;
};
