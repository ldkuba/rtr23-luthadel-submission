#include "renderer/vulkan/vulkan_image.hpp"

VulkanImage::~VulkanImage() {
    if (_handle) _device->handle().destroyImage(_handle, _allocator);
    if (_memory) _device->handle().freeMemory(_memory, _allocator);
    if (_has_view) _device->handle().destroyImageView(view, _allocator);
}

// /////////////////////////// //
// VULKAN IMAGE PUBLIC METHODS //
// /////////////////////////// //

void VulkanImage::create(
    const uint32 width,
    const uint32 height,
    const uint32 mip_levels,
    const vk::SampleCountFlagBits number_of_samples,
    const vk::Format format,
    const vk::ImageTiling tiling,
    const vk::ImageUsageFlags usage,
    const vk::MemoryPropertyFlags properties
) {
    // Remember width and height
    this->_width = width;
    this->_height = height;

    // Create image
    vk::ImageCreateInfo image_info{};
    image_info.setImageType(vk::ImageType::e2D); // This class only covers 2D images
    image_info.extent.setWidth(width);           // Image width
    image_info.extent.setHeight(height);         // Image height
    image_info.extent.setDepth(1);               // Image depth (always 1 for 2D)
    image_info.setMipLevels(mip_levels);         // Maximum number of mipmaping levels
    // TODO: Should be configurable
    image_info.setArrayLayers(1);                // Number of image layers
    image_info.setFormat(format);                // Image format used
    image_info.setTiling(tiling);                // Image tiling used (Liner or Optimal)
    // Either Undefined or Predefined. Determins whether the firs transition will preserve the texels
    image_info.setInitialLayout(vk::ImageLayout::eUndefined);
    image_info.setUsage(usage);                  // Specifies purpose of the image data
    // Determins ownership between Queue families (Exclusive or Concurrent)
    image_info.setSharingMode(vk::SharingMode::eExclusive);
    image_info.setSamples(number_of_samples);    // Number of samples used for MSAA

    try {
        _handle = _device->handle().createImage(image_info, _allocator);
    } catch (vk::SystemError e) { Logger::log(e.what()); }

    // Allocate image memory
    auto memory_requirements = _device->handle().getImageMemoryRequirements(handle);

    vk::MemoryAllocateInfo allocation_info{};
    // Number of bytes to be allocated
    allocation_info.setAllocationSize(memory_requirements.size);
    // Type of memory we wish to allocate from
    allocation_info.setMemoryTypeIndex(
        _device->find_memory_type(memory_requirements.memoryTypeBits, properties));

    try {
        _memory = _device->handle().allocateMemory(allocation_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    // Bind memory to the created image
    _device->handle().bindImageMemory(handle, memory, 0);
}

void VulkanImage::create(
    const uint32 width,
    const uint32 height,
    const uint32 mip_levels,
    const vk::SampleCountFlagBits number_of_samples,
    const vk::Format format,
    const vk::ImageTiling tiling,
    const vk::ImageUsageFlags usage,
    const vk::MemoryPropertyFlags properties,
    const vk::ImageAspectFlags aspect_flags
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
    const vk::Image image,
    const uint32 mip_levels,
    const vk::Format format,
    const vk::ImageAspectFlags aspect_flags
) {
    handle = image;
    create_view(mip_levels, format, aspect_flags);
}

void VulkanImage::transition_image_layout(
    VulkanCommandPool* const command_pool,
    const vk::Format format,
    const vk::ImageLayout old_layout,
    const vk::ImageLayout new_layout,
    const uint32 mip_levels
) const {
    auto command_buffer = command_pool->begin_single_time_commands();

    // Implement transition barrier
    vk::ImageMemoryBarrier barrier{};
    barrier.setImage(handle);         // Image to transfer
    barrier.setOldLayout(old_layout); // From Layout
    barrier.setNewLayout(new_layout); // To layout
    // Transfers the image from one queue to the other (When the image is exclusively owned)
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    // Image aspect effected by the transition
    barrier.subresourceRange.setAspectMask(_aspect_flags);
    // Samples effected
    barrier.subresourceRange.setBaseMipLevel(0);
    barrier.subresourceRange.setLevelCount(mip_levels);
    // Layers effected
    barrier.subresourceRange.setBaseArrayLayer(0);
    barrier.subresourceRange.setLayerCount(1);

    vk::PipelineStageFlags source_stage, destination_stage;
    if (old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eTransferDstOptimal) {
        // Operations that need to be completed before the transition
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        // Operations that need to wait for transition
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        // In which pipeline stage do operations we wait on occur
        source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        // In which pipeline stage do operations wait on transition
        destination_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (
        old_layout == vk::ImageLayout::eTransferDstOptimal &&
        new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        source_stage = vk::PipelineStageFlagBits::eTransfer;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else
        throw std::invalid_argument("Unsupported layout transition.");

    // Transition image layout with barrier
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
    const vk::Format format,
    const vk::ImageAspectFlags aspect_flags,
    const vk::Image& image,
    const vk::Device& device,
    const vk::AllocationCallbacks* const allocator
) {
    // Construct image view
    vk::ImageViewCreateInfo create_info{};
    create_info.setImage(image);                              // Image for which we are creating a view
    create_info.setViewType(vk::ImageViewType::e2D);          // 2D image
    create_info.setFormat(format);                            // Image format
    create_info.subresourceRange.setAspectMask(aspect_flags); // Image aspect (eg. color, depth...)
    // Mipmaping
    create_info.subresourceRange.setBaseMipLevel(0);          // Level to start mipmaping from
    create_info.subresourceRange.setLevelCount(1);            // Maximum number of mipmaping levels
    // Image array
    create_info.subresourceRange.setBaseArrayLayer(0);
    create_info.subresourceRange.setLayerCount(1);

    try {
        return device.createImageView(create_info, allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
    return vk::ImageView();
}

// //////////////////////////// //
// VULKAN IMAGE PRIVATE METHODS //
// //////////////////////////// //

void VulkanImage::create_view(
    const uint32 mip_levels,
    const vk::Format format,
    const vk::ImageAspectFlags aspect_flags
) {
    _has_view = true;
    _aspect_flags = aspect_flags;
    // Construct image view
    vk::ImageViewCreateInfo create_info{};
    create_info.setImage(_handle);                             // Image for which we are creating a view
    create_info.setViewType(vk::ImageViewType::e2D);          // 2D image
    create_info.setFormat(format);                            // Image format
    create_info.subresourceRange.setAspectMask(aspect_flags); // Image aspect (eg. color, depth...)
    // Mipmaping
    create_info.subresourceRange.setBaseMipLevel(0);          // Level to start mipmaping from
    create_info.subresourceRange.setLevelCount(mip_levels);   // Maximum number of mipmaping levels
    // Image array
    create_info.subresourceRange.setBaseArrayLayer(0);
    create_info.subresourceRange.setLayerCount(1);

    try {
        _view = _device->handle().createImageView(create_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }
}