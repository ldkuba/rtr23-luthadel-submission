#include "renderer/vulkan/vulkan_buffer.hpp"

VulkanBuffer::~VulkanBuffer() {
    if (handle) _device->handle.destroyBuffer(handle, _allocator);
    if (memory) _device->handle.freeMemory(memory, _allocator);
}

void VulkanBuffer::create(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    bool bind_on_create
) {
    this->size = size;
    _properties = properties;
    _usage = usage;

    // Create buffer
    vk::BufferCreateInfo buffer_info{};
    buffer_info.setSize(size);
    buffer_info.setUsage(usage);
    buffer_info.setSharingMode(vk::SharingMode::eExclusive); // Used only in one queue

    try {
        handle = _device->handle.createBuffer(buffer_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }

    // Allocate memory to the buffer
    auto memory_requirements = _device->handle.getBufferMemoryRequirements(handle);

    vk::MemoryAllocateInfo allocation_info{};
    allocation_info.setAllocationSize(memory_requirements.size);
    allocation_info.setMemoryTypeIndex(
        _device->find_memory_type(memory_requirements.memoryTypeBits, properties));

    try {
        memory = _device->handle.allocateMemory(allocation_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }

    // Bind allocated memory to buffer
    if (bind_on_create)
        _device->handle.bindBufferMemory(handle, memory, 0);
}

void VulkanBuffer::bind(vk::DeviceSize offset) {
    _device->handle.bindBufferMemory(handle, memory, offset);
}

void VulkanBuffer::resize(
    VulkanCommandPool* command_pool,
    vk::DeviceSize new_size
) {
    // Create new buffer
    vk::BufferCreateInfo buffer_info{};
    buffer_info.setSize(new_size);
    buffer_info.setUsage(_usage);
    buffer_info.setSharingMode(vk::SharingMode::eExclusive);

    vk::Buffer new_handle;
    try {
        new_handle = _device->handle.createBuffer(buffer_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }

    // Allocate memory to the buffer
    auto memory_requirements = _device->handle.getBufferMemoryRequirements(new_handle);

    vk::MemoryAllocateInfo allocation_info{};
    allocation_info.setAllocationSize(memory_requirements.size);
    allocation_info.setMemoryTypeIndex(
        _device->find_memory_type(memory_requirements.memoryTypeBits, _properties));

    vk::DeviceMemory new_memory;
    try {
        new_memory = _device->handle.allocateMemory(allocation_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(e.what()); }

    // Bind allocated memory to buffer
    _device->handle.bindBufferMemory(new_handle, new_memory, 0);

    // Copy data over
    copy_data_to_buffer(command_pool, new_handle, 0, 0, size);

    // Destroy the old
    if (handle) _device->handle.destroyBuffer(handle, _allocator);
    if (memory) _device->handle.freeMemory(memory, _allocator);

    // Set new properties
    size = new_size;
    handle = new_handle;
    memory = new_memory;
}

void VulkanBuffer::load_data(
    const void* data,
    vk::DeviceSize offset,
    vk::DeviceSize size,
    vk::MemoryMapFlags flags
) {
    auto data_ptr = _device->handle.mapMemory(memory, offset, size, flags);
    memcpy(data_ptr, data, (size_t) size);
    _device->handle.unmapMemory(memory);
}

void VulkanBuffer::copy_data_to_buffer(
    VulkanCommandPool* command_pool,
    vk::Buffer& buffer,
    vk::DeviceSize source_offset,
    vk::DeviceSize destination_offset,
    vk::DeviceSize size
) {
    auto command_buffer = command_pool->begin_single_time_commands();

    // Copy regions
    vk::BufferCopy copy_region;
    copy_region.setSrcOffset(source_offset);
    copy_region.setDstOffset(destination_offset);
    copy_region.setSize(size);

    command_buffer.copyBuffer(handle, buffer, 1, &copy_region);

    command_pool->end_single_time_commands(command_buffer);
}

void VulkanBuffer::copy_data_to_image(
    VulkanCommandPool* command_pool,
    VulkanImage* image
) {
    auto command_buffer = command_pool->begin_single_time_commands();

    // Preform copy ops
    vk::BufferImageCopy region{};
    // Buffer info
    region.setBufferOffset(0);
    region.setBufferRowLength(0);
    region.setBufferImageHeight(0);
    // Image info
    region.imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
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

    command_pool->end_single_time_commands(command_buffer);
}