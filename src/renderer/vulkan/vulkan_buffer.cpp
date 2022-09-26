#include "renderer/vulkan/vulkan_buffer.hpp"

VulkanBuffer::~VulkanBuffer() {
    if (_handle) _device->handle().destroyBuffer(_handle, _allocator);
    if (_memory) _device->handle().freeMemory(_memory, _allocator);
}

// //////////////////////////// //
// VULKAN BUFFER PUBLIC METHODS //
// //////////////////////////// //

void VulkanBuffer::create(
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage,
    const vk::MemoryPropertyFlags properties,
    const bool bind_on_create
) {
    this->_size = size;
    _properties = properties;
    _usage = usage;

    // Create buffer
    _handle = create_buffer(size, usage);

    // Allocate memory to the buffer
    _memory = allocate_buffer_memory(_handle, properties);

    // Bind allocated memory to the buffer if required
    if (bind_on_create) _device->handle().bindBufferMemory(_handle, _memory, 0);
}

void VulkanBuffer::bind(const vk::DeviceSize offset) const {
    if (_handle) _device->handle().bindBufferMemory(_handle, _memory, offset);
}

void VulkanBuffer::resize(
    const vk::CommandBuffer& command_buffer,
    const vk::DeviceSize new_size
) {
    // Create new buffer with new required size
    vk::Buffer new_handle = create_buffer(new_size, _usage);

    // Allocate memory to new buffer
    vk::DeviceMemory new_memory = allocate_buffer_memory(new_handle, _properties);

    // Bind allocated memory to buffer
    _device->handle().bindBufferMemory(new_handle, new_memory, 0);

    // Copy all the data over
    copy_data_to_buffer(command_buffer, new_handle, 0, 0, size);

    // Destroy the old
    if (_handle) _device->handle().destroyBuffer(_handle, _allocator);
    if (_memory) _device->handle().freeMemory(_memory, _allocator);

    // Set the new
    _size = new_size;
    _handle = new_handle;
    _memory = new_memory;
}

void VulkanBuffer::load_data(
    const void* const data,
    const vk::DeviceSize offset,
    const vk::DeviceSize size
) const {
    // Map buffer memory into a CPU accessible memory
    auto data_ptr = _device->handle().mapMemory(memory, offset, size);
    memcpy(data_ptr, data, (size_t) size); // Copy data to the assigned memory
    _device->handle().unmapMemory(memory);   // Unmap the memory
}

void VulkanBuffer::copy_data_to_buffer(
    const vk::CommandBuffer& command_buffer,
    const vk::Buffer& buffer,
    const vk::DeviceSize source_offset,
    const vk::DeviceSize destination_offset,
    const vk::DeviceSize size
) const {
    // Copy regions
    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(source_offset);
    copy_region.setDstOffset(destination_offset);
    copy_region.setSize(size);

    command_buffer.copyBuffer(handle, buffer, 1, &copy_region);
}

void VulkanBuffer::copy_data_to_image(
    const vk::CommandBuffer& command_buffer,
    VulkanImage* const image,
    const vk::ImageAspectFlags image_aspect
) const {
    // Preform copy ops
    vk::BufferImageCopy region{};
    // Buffer info
    region.setBufferOffset(0);      // Byte offset in the buffer at which pixels start
    region.setBufferRowLength(0);   // Specifies how the pixels are laid in memory
    region.setBufferImageHeight(0); // 0 for these 2 fields indicates  tightly packed pixels
    // Image info
    region.imageSubresource.setAspectMask(image_aspect);
    region.imageSubresource.setMipLevel(0);
    region.imageSubresource.setBaseArrayLayer(0);
    region.imageSubresource.setLayerCount(1);
    region.setImageOffset({ 0,0,0 });
    region.setImageExtent({ image->width, image->height, 1 });

    command_buffer.copyBufferToImage(
        handle, image->handle,
        vk::ImageLayout::eTransferDstOptimal,
        1, &region
    );
}

// /////////////////////////////// //
// VULKAN BUFFER PRIVATE FUNCTIONS //
// /////////////////////////////// //

vk::Buffer VulkanBuffer::create_buffer(
    const vk::DeviceSize size,
    const vk::BufferUsageFlags usage
) const {
    vk::BufferCreateInfo buffer_info{};
    buffer_info.setSize(size);   // Buffer size in bytes
    buffer_info.setUsage(usage); // Specifies for which purpose the buffer data will be used
    buffer_info.setSharingMode(vk::SharingMode::eExclusive); // Used only in one queue

    vk::Buffer buffer;
    try {
        buffer = _device->handle().createBuffer(buffer_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }
    return buffer;
}

vk::DeviceMemory VulkanBuffer::allocate_buffer_memory(
    const vk::Buffer buffer,
    const vk::MemoryPropertyFlags properties
) const {
    auto memory_requirements = _device->handle().getBufferMemoryRequirements(buffer);

    vk::MemoryAllocateInfo allocation_info{};
    // Number of bytes to be allocated
    allocation_info.setAllocationSize(memory_requirements.size);
    // Type of memory we wish to allocate from
    allocation_info.setMemoryTypeIndex(
        _device->find_memory_type(memory_requirements.memoryTypeBits, properties));

    vk::DeviceMemory memory;
    try {
        memory = _device->handle().allocateMemory(allocation_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }
    return memory;
}