#pragma once

#include "vulkan_image.hpp"

class VulkanBuffer {
public:
    /// @brief Handle to the vk:Buffer
    Property<vk::Buffer> handle{ Get { return _handle; } };
    /// @brief Pointer to on device memory of the allocated buffer
    Property<vk::DeviceMemory> memory{ Get { return _memory;} };
    /// @brief Total buffer size in bytes
    Property<vk::DeviceSize> size{ Get { return _size; } };

    VulkanBuffer(const VulkanDevice* const device, const vk::AllocationCallbacks* const allocator)
        : _device(device), _allocator(allocator) {}
    ~VulkanBuffer();

    /// @brief Create and allocate on device memory for this buffer
    /// @param size Required buffer size
    /// @param usage Buffer usage flags
    /// @param properties Required properties for on device memory
    /// @param bind_on_create Binds created buffer to the allocated memory if true, default = true
    void create(
        const vk::DeviceSize size,
        const vk::BufferUsageFlags usage,
        const vk::MemoryPropertyFlags properties,
        const bool bind_on_create = true
    );

    /// @brief Bind buffer to memory
    /// @param offset Offset at which the bind should start at
    void bind(const vk::DeviceSize offset) const;

    /// @brief Resize buffer
    /// @param command_pool Command pool to witch the resize command will be submitted
    /// @param new_size New buffer size in bytes
    void resize(
        VulkanCommandPool* const command_pool,
        const vk::DeviceSize new_size
    );

    /// @brief Upload data to buffer
    /// @param data Byte array to be uploaded
    /// @param offset Data offset
    /// @param size Total data size in bytes
    void load_data(
        const void* const data,
        const vk::DeviceSize offset,
        const vk::DeviceSize size
    ) const;

    /// @brief Copy buffer data to another buffer
    /// @param command_pool Command pool to which the transfer command will be submitted
    /// @param buffer Other buffer
    /// @param source_offset Source memory offset
    /// @param destination_offset Destination memory offset
    /// @param size Number of bytes copied
    void copy_data_to_buffer(
        VulkanCommandPool* const command_pool,
        const vk::Buffer& buffer,
        const vk::DeviceSize source_offset,
        const vk::DeviceSize destination_offset,
        const vk::DeviceSize size
    ) const;

    /// @brief Copy buffer data to an image
    /// @param command_pool Command pool to which the transfer command will be submitted
    /// @param image Image to which we are transferring data
    /// @param image_aspect Image aspect to which we are transferring data, default = color
    void copy_data_to_image(
        VulkanCommandPool* const command_pool,
        VulkanImage* const image,
        const vk::ImageAspectFlags image_aspect = vk::ImageAspectFlagBits::eColor
    ) const;

private:

    const VulkanDevice* _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::Buffer _handle;
    vk::DeviceMemory _memory;
    vk::DeviceSize _size;
    vk::BufferUsageFlags _usage;
    vk::MemoryPropertyFlags _properties;

    vk::Buffer create_buffer(
        const vk::DeviceSize size,
        const vk::BufferUsageFlags usage
    ) const;
    vk::DeviceMemory allocate_buffer_memory(
        const vk::Buffer buffer,
        const vk::MemoryPropertyFlags properties
    ) const;
};