#pragma once

#include "vulkan_buffer.hpp"
#include "systems/memory/memory_allocators/gpu_free_list_allocator.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Managed specification of VulkanBuffer. Utilizes a client side Free
 * list allocation method for on-device memory management of the buffer it
 * allocates.
 */
class VulkanManagedBuffer : public VulkanBuffer {
  public:
    using VulkanBuffer::VulkanBuffer;

    /**
     * @brief Construct a new Vulkan Managed Buffer object
     *
     * @param device Vulkan device reference
     * @param allocator Allocation callback used
     */
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
    ) override;

    /// @brief Resize buffer
    /// @param command_buffer Command buffer to witch the resize command will be
    /// submitted
    /// @param new_size New buffer size in bytes
    void resize(
        const vk::CommandBuffer& command_buffer, const vk::DeviceSize new_size
    ) override;

    /// @brief Upload data to buffer. If the region denoted by the offset and
    /// size isn't fully allocated raises segmentation error.
    /// @param data Byte array to be uploaded
    /// @param offset Data offset
    /// @param size Total data size in bytes
    void load_data(
        const void* const    data,
        const vk::DeviceSize offset,
        const vk::DeviceSize size
    ) const override;

    /// @brief Allocates buffer memory.
    /// @param size Requested allocation size
    /// @returns In buffer offset at which the allocated region starts.
    vk::DeviceSize allocate(const uint64 size, const uint64 alignment = 8);

    /// @brief Deallocates part of the buffer memory.
    /// @param offset In buffer offset at which the allocated region starts.
    void deallocate(vk::DeviceSize offset);

  private:
    GPUFreeListAllocator* _memory_allocator;
};

} // namespace ENGINE_NAMESPACE