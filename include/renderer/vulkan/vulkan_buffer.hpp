#pragma once

#include "vulkan_image.hpp"

/**
 * @brief Vulkan specific data buffer
 */
class VulkanBuffer {
  public:
    /// @brief Handle to the vk:Buffer
    Property<vk::Buffer> handle {
        GET { return _handle; }
    };
    /// @brief Pointer to on device memory of the allocated buffer
    Property<vk::DeviceMemory> memory {
        GET { return _memory; }
    };
    /// @brief Total buffer size in bytes
    Property<vk::DeviceSize> size {
        GET { return _size; }
    };

    /**
     * @brief Construct a new Vulkan Buffer object
     *
     * @param device Vulkan device reference
     * @param allocator Allocation callback used
     */
    VulkanBuffer(
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator
    )
        : _device(device), _allocator(allocator) {}
    virtual ~VulkanBuffer();

    /// @brief Create and allocate on device memory for this buffer
    /// @param size Required buffer size
    /// @param usage Buffer usage flags
    /// @param properties Required properties for on device memory
    /// @param bind_on_create Binds created buffer to the allocated memory if
    /// true, default = true
    virtual void create(
        const vk::DeviceSize          size,
        const vk::BufferUsageFlags    usage,
        const vk::MemoryPropertyFlags properties,
        const bool                    bind_on_create = true
    );

    /// @brief Bind buffer to memory
    /// @param offset Offset at which the bind should start at
    virtual void bind(const vk::DeviceSize offset) const;

    /// @brief Resize buffer. Only works for increased buffer the size.
    /// @param command_buffer Command buffer to witch the resize command will be
    /// submitted
    /// @param new_size New buffer size in bytes
    virtual void resize(
        const vk::CommandBuffer& command_buffer, const vk::DeviceSize new_size
    );

    /// @brief Upload data to buffer
    /// @param data Byte array to be uploaded
    /// @param offset Data offset
    /// @param size Total data size in bytes
    virtual void load_data(
        const void* const    data,
        const vk::DeviceSize offset,
        const vk::DeviceSize size
    ) const;

    /// @brief Locks (or maps) the buffer memory to a temporary location of host
    /// memory, which should be unlocked before shutdown or destruction.
    /// @param offset An offset in bytes to lock the memory at
    /// @param size The amount of memory to lock
    /// @return A pointer to a block of memory, mapped to the buffer's memory
    virtual void* lock_memory(
        const vk::DeviceSize offset, const vk::DeviceSize size
    );

    /// @brief Unlocks (or unmaps) the buffer memory.
    virtual void unlock_memory();

    /// @brief Copy buffer data to another buffer
    /// @param command_buffer Command buffer to which the transfer command will
    /// be submitted
    /// @param buffer Other buffer
    /// @param source_offset Source memory offset
    /// @param destination_offset Destination memory offset
    /// @param size Number of bytes copied
    virtual void copy_data_to_buffer(
        const vk::CommandBuffer& command_buffer,
        const vk::Buffer&        buffer,
        const vk::DeviceSize     source_offset,
        const vk::DeviceSize     destination_offset,
        const vk::DeviceSize     size
    ) const;

    /// @brief Copy buffer data to an image
    /// @param command_buffer Command buffer to which the transfer command will
    /// be submitted
    /// @param image Image to which we are transferring data
    /// @param image_aspect Image aspect to which we are transferring data,
    /// default = color
    virtual void copy_data_to_image(
        const vk::CommandBuffer&   command_buffer,
        VulkanImage* const         image,
        const vk::ImageAspectFlags image_aspect =
            vk::ImageAspectFlagBits::eColor
    ) const;

  private:

    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::Buffer              _handle;
    vk::DeviceMemory        _memory;
    vk::DeviceSize          _size;
    vk::BufferUsageFlags    _usage;
    vk::MemoryPropertyFlags _properties;

    vk::Buffer create_buffer(
        const vk::DeviceSize size, const vk::BufferUsageFlags usage
    ) const;
    vk::DeviceMemory allocate_buffer_memory(
        const vk::Buffer buffer, const vk::MemoryPropertyFlags properties
    ) const;
};