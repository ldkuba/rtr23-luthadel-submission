#include "renderer/vulkan/vulkan_image.hpp"

#include "logger.hpp"

VulkanImage::~VulkanImage() {
    _device->handle.destroyImage(handle, _allocator);
    _device->handle.freeMemory(memory, _allocator);
    if (_has_view) _device->handle.destroyImageView(view, _allocator);
}

// Image functions
void VulkanImage::create(
    uint32 width,
    uint32 height,
    uint32 mip_levels,
    vk::SampleCountFlagBits number_of_samples,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties
) {
    // Remember width and height
    this->width = width;
    this->height = height;

    // Create image
    vk::ImageCreateInfo image_info{};
    image_info.setImageType(vk::ImageType::e2D);
    image_info.extent.setWidth(width);
    image_info.extent.setHeight(height);
    image_info.extent.setDepth(1);
    image_info.setMipLevels(mip_levels);
    image_info.setArrayLayers(1);   // TODO: Should be configurable
    image_info.setFormat(format);
    image_info.setTiling(tiling);
    image_info.setInitialLayout(vk::ImageLayout::eUndefined);
    image_info.setUsage(usage);
    image_info.setSharingMode(vk::SharingMode::eExclusive);
    image_info.setSamples(number_of_samples);

    try {
        handle = _device->handle.createImage(image_info, _allocator);
    } catch (vk::SystemError e) { Logger::log(e.what()); }

    // Allocate image memory
    auto memory_requirements = _device->handle.getImageMemoryRequirements(handle);

    vk::MemoryAllocateInfo allocation_info{};
    allocation_info.setAllocationSize(memory_requirements.size);
    allocation_info.setMemoryTypeIndex(_device->find_memory_type(memory_requirements.memoryTypeBits, properties));

    try {
        memory = _device->handle.allocateMemory(allocation_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    _device->handle.bindImageMemory(handle, memory, 0);
}

void VulkanImage::create(
    uint32 width,
    uint32 height,
    uint32 mip_levels,
    vk::SampleCountFlagBits number_of_samples,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::ImageAspectFlags aspect_flags
) {
    // Construct image
    create(
        width,
        height,
        mip_levels,
        number_of_samples,
        format,
        tiling,
        usage,
        properties
    );

    // Construct image view
    create_view(
        mip_levels,
        format,
        aspect_flags
    );
}

void VulkanImage::create(
    vk::Image image,
    uint32 mip_levels,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags
) {
    handle = image;
    create_view(mip_levels, format, aspect_flags);
}

void VulkanImage::create_view(
    uint32 mip_levels,
    vk::Format format,
    vk::ImageAspectFlags aspect_flags
) {
    // Construct image view
    _has_view = true;
    vk::ImageViewCreateInfo create_info{};
    create_info.setImage(handle);                             // Image for which we are creating a view
    create_info.setViewType(vk::ImageViewType::e2D);          // 2D image
    create_info.setFormat(format);                            // Image format
    create_info.subresourceRange.setAspectMask(aspect_flags); // Image aspect (eg. color, depth...)
    // Mipmaping
    create_info.subresourceRange.setBaseMipLevel(0);
    create_info.subresourceRange.setLevelCount(mip_levels);
    // ...
    create_info.subresourceRange.setBaseArrayLayer(0);
    create_info.subresourceRange.setLayerCount(1);

    try {
        view = _device->handle.createImageView(create_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
}

void VulkanImage::transition_image_layout(
    VulkanCommandPool* command_pool,
    vk::Format format,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    uint32 mip_levels
) {
    auto command_buffer = command_pool->begin_single_time_commands();

    // Implement transition barrier
    vk::ImageMemoryBarrier barrier{};
    barrier.setOldLayout(old_layout);
    barrier.setNewLayout(new_layout);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(handle);
    barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    barrier.subresourceRange.setBaseMipLevel(0);
    barrier.subresourceRange.setLevelCount(mip_levels);
    barrier.subresourceRange.setBaseArrayLayer(0);
    barrier.subresourceRange.setLayerCount(1);

    vk::PipelineStageFlags source_stage, destination_stage;

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        source_stage = vk::PipelineStageFlagBits::eTransfer;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else
        throw std::invalid_argument("Unsupported layout transition.");

    command_buffer.pipelineBarrier(
        source_stage, destination_stage,
        vk::DependencyFlags(),
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    command_pool->end_single_time_commands(command_buffer);
}

vk::ImageView VulkanImage::get_view_from_image(
    vk::Format format,
    vk::ImageAspectFlags aspect_flags,
    const vk::Image& image,
    const vk::Device& device,
    const vk::AllocationCallbacks* allocator
) {
    // Construct image view
    vk::ImageViewCreateInfo create_info{};
    create_info.setImage(image);                             // Image for which we are creating a view
    create_info.setViewType(vk::ImageViewType::e2D);          // 2D image
    create_info.setFormat(format);                            // Image format
    create_info.subresourceRange.setAspectMask(aspect_flags); // Image aspect (eg. color, depth...)
    // Mipmaping
    create_info.subresourceRange.setBaseMipLevel(0);
    create_info.subresourceRange.setLevelCount(1);
    // ...
    create_info.subresourceRange.setBaseArrayLayer(0);
    create_info.subresourceRange.setLayerCount(1);

    try {
        return device.createImageView(create_info, allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
    return vk::ImageView();
}