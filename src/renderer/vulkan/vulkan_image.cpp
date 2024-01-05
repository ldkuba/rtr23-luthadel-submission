#include "renderer/vulkan/vulkan_image.hpp"

namespace ENGINE_NAMESPACE {

// #define TRACE_FILE_VULKAN_IMAGE

VulkanImage::~VulkanImage() {
    if (_handle) _device->handle().destroyImage(_handle, _allocator);
    if (_memory) _device->handle().freeMemory(_memory, _allocator);
    destroy_view();
#ifdef TRACE_FILE_VULKAN_IMAGE
    Logger::trace(RENDERER_VULKAN_LOG, "Image destroyed.");
#endif
}

// /////////////////////////// //
// VULKAN IMAGE PUBLIC METHODS //
// /////////////////////////// //

void VulkanImage::create(
    const vk::Image handle, const uint32 width, const uint32 height
) {
    _handle = handle;
    _width  = width;
    _height = height;
}

void VulkanImage::create_2d(
    const uint32                              width,
    const uint32                              height,
    const uint8                               mip_levels,
    const vk::SampleCountFlagBits             number_of_samples,
    const vk::Format                          format,
    const vk::ImageTiling                     tiling,
    const vk::ImageUsageFlags                 usage,
    const vk::MemoryPropertyFlags             properties,
    const std::optional<vk::ImageAspectFlags> aspect_flags
) {
#ifdef TRACE_FILE_VULKAN_IMAGE
    Logger::trace(RENDERER_VULKAN_LOG, "Creating image.");
#endif

    // Construct image
    create_internal(
        vk::ImageType::e2D,
        width,
        height,
        1,
        mip_levels,
        1,
        number_of_samples,
        format,
        tiling,
        usage,
        properties
    );

    // Construct image view
    _view_type = vk::ImageViewType::e2D;
    if (aspect_flags.has_value()) create_view(aspect_flags.value());

#ifdef TRACE_FILE_VULKAN_IMAGE
    Logger::trace(RENDERER_VULKAN_LOG, "Image created.");
#endif
}

void VulkanImage::create_cube(
    const uint32                              width,
    const uint32                              height,
    const uint8                               mip_levels,
    const vk::SampleCountFlagBits             number_of_samples,
    const vk::Format                          format,
    const vk::ImageTiling                     tiling,
    const vk::ImageUsageFlags                 usage,
    const vk::MemoryPropertyFlags             properties,
    const std::optional<vk::ImageAspectFlags> aspect_flags
) {
    // Create images
    create_internal(
        vk::ImageType::e2D,
        width,
        height,
        1,
        mip_levels,
        6,
        number_of_samples,
        format,
        tiling,
        usage,
        properties,
        vk::ImageCreateFlagBits::eCubeCompatible
    );

    // Construct image view
    _view_type = vk::ImageViewType::eCube;
    if (aspect_flags.has_value()) create_view(aspect_flags.value());
}

void VulkanImage::create(
    const vk::Image            image,
    const uint8                mip_levels,
    const vk::Format           format,
    const vk::ImageAspectFlags aspect_flags
) {
#ifdef TRACE_FILE_VULKAN_IMAGE
    Logger::trace(RENDERER_VULKAN_LOG, "Creating image.");
#endif

    handle = image;
    create_view(aspect_flags);

#ifdef TRACE_FILE_VULKAN_IMAGE
    Logger::trace(RENDERER_VULKAN_LOG, "Image created.");
#endif
}

void VulkanImage::create_view(const vk::ImageAspectFlags aspect_flags) {
    _has_view     = true;
    _aspect_flags = aspect_flags;

    // Construct image view
    vk::ImageViewCreateInfo create_info {};
    create_info.setImage(_handle); // Image for which we are creating a view
    create_info.setViewType(_view_type); // 2D / 3D / Cube... image
    create_info.setFormat(_format);      // Image format
    create_info.subresourceRange.setAspectMask(aspect_flags
    ); // Image aspect (eg. color, depth...)
    // Mipmaping
    create_info.subresourceRange.setBaseMipLevel(0
    ); // Level to start mipmaping from
    create_info.subresourceRange.setLevelCount(_mip_levels
    ); // Maximum number of mipmaping levels
    // Image array
    create_info.subresourceRange.setBaseArrayLayer(0);
    create_info.subresourceRange.setLayerCount(_array_layers);

    try {
        _view = _device->handle().createImageView(create_info, _allocator);
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }
}

void VulkanImage::destroy_view() {
    if (!_has_view) return;
    _device->handle().destroyImageView(_view, _allocator);
    _has_view = false;
}

Result<void, InvalidArgument> VulkanImage::transition_image_layout(
    const vk::CommandBuffer& command_buffer,
    const vk::ImageLayout    old_layout,
    const vk::ImageLayout    new_layout
) const {
    // Implement transition barrier
    vk::ImageMemoryBarrier barrier {};
    barrier.setImage(handle);         // Image to transfer
    barrier.setOldLayout(old_layout); // From Layout
    barrier.setNewLayout(new_layout); // To layout
    // Transfers the image from one queue to the other (When the image is
    // exclusively owned)
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    // Image aspect effected by the transition
    barrier.subresourceRange.setAspectMask(_aspect_flags);
    // Samples effected
    barrier.subresourceRange.setBaseMipLevel(0);
    barrier.subresourceRange.setLevelCount(_mip_levels);
    // Layers effected
    barrier.subresourceRange.setBaseArrayLayer(0);
    barrier.subresourceRange.setLayerCount(_array_layers);

    vk::PipelineStageFlags source_stage, destination_stage;
    if (old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eTransferDstOptimal) {
        // Operations that need to be completed before the transition
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        // Operations that need to wait for transition
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        // In which pipeline stage do operations we wait on occur
        source_stage      = vk::PipelineStageFlagBits::eTopOfPipe;
        // In which pipeline stage do operations wait on transition
        destination_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if ( //
        old_layout == vk::ImageLayout::eTransferDstOptimal &&
        new_layout == vk::ImageLayout::eShaderReadOnlyOptimal
    ) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        source_stage      = vk::PipelineStageFlagBits::eTransfer;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if ( //
        old_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal &&
        new_layout == vk::ImageLayout::eDepthStencilReadOnlyOptimal
    ) {
        barrier.setSrcAccessMask(
            vk::AccessFlagBits::eDepthStencilAttachmentWrite
        );
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        source_stage      = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if ( //
        old_layout == vk::ImageLayout::eColorAttachmentOptimal &&
        new_layout == vk::ImageLayout::eShaderReadOnlyOptimal
    ) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        source_stage      = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if ( //
        old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eShaderReadOnlyOptimal
    ) {
        barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        source_stage      = vk::PipelineStageFlagBits::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else return Failure("Unsupported layout transition.");

    // Transition image layout with barrier
    command_buffer.pipelineBarrier(
        source_stage,
        destination_stage,
        vk::DependencyFlags(),
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
    return {};
}

void VulkanImage::generate_mipmaps(const vk::CommandBuffer& command_buffer
) const {
    // Check if image format supports linear blitting
    auto properties = _device->info().get_format_properties(_format);
    if (!(properties.optimalTilingFeatures &
          vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Texture image format does not support linear blitting."
        );

    // Generate mipmaps
    vk::ImageMemoryBarrier barrier {};
    barrier.setImage(_handle);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.subresourceRange.setAspectMask(_aspect_flags);
    barrier.subresourceRange.setBaseArrayLayer(0);
    barrier.subresourceRange.setLayerCount(_array_layers);
    barrier.subresourceRange.setLevelCount(1);

    uint32 mip_width  = _width;
    uint32 mip_height = _height;
    for (uint32 i = 1; i < _mip_levels; i++) {
        // Transition bitmap layer i-1 to transfer optimal layout
        barrier.subresourceRange.setBaseMipLevel(i - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags(),
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        // Create bitmap level
        vk::ImageBlit blit {};
        blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.srcOffsets[1] = vk::Offset3D(mip_width, mip_height, 1);
        blit.srcSubresource.setAspectMask(_aspect_flags);
        blit.srcSubresource.setMipLevel(i - 1);
        blit.srcSubresource.setBaseArrayLayer(0);
        blit.srcSubresource.setLayerCount(_array_layers);
        blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.dstOffsets[1] = vk::Offset3D(
            mip_width > 1 ? mip_width / 2 : 1,
            mip_height > 1 ? mip_height / 2 : 1,
            1
        );
        blit.dstSubresource.setAspectMask(_aspect_flags);
        blit.dstSubresource.setMipLevel(i);
        blit.dstSubresource.setBaseArrayLayer(0);
        blit.dstSubresource.setLayerCount(_array_layers);

        command_buffer.blitImage(
            _handle,
            vk::ImageLayout::eTransferSrcOptimal,
            _handle,
            vk::ImageLayout::eTransferDstOptimal,
            1,
            &blit,
            vk::Filter::eLinear
        );

        // Transition bitmap layer i-1 to read only optimal layout
        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
        barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags(),
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        // Half mipmap resolution for next iteration
        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }

    // Transition last bitmap layer to read only optimal layout
    barrier.subresourceRange.setBaseMipLevel(_mip_levels - 1);
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlags(),
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
}

// //////////////////////////// //
// VULKAN IMAGE PRIVATE METHODS //
// //////////////////////////// //

void VulkanImage::create_internal(
    const vk::ImageType           type,
    const uint32                  width,
    const uint32                  height,
    const uint32                  depth,
    const uint8                   mip_levels,
    const uint8                   array_layers,
    const vk::SampleCountFlagBits number_of_samples,
    const vk::Format              format,
    const vk::ImageTiling         tiling,
    const vk::ImageUsageFlags     usage,
    const vk::MemoryPropertyFlags properties,
    const vk::ImageCreateFlags    flags
) {
    // Remember properties
    _type              = type;
    _width             = width;
    _height            = height;
    _depth             = depth;
    _mip_levels        = mip_levels;
    _array_layers      = array_layers;
    _number_of_samples = number_of_samples;
    _format            = format;
    _tiling            = tiling;
    _usage             = usage;
    _properties        = properties;

    // Create image
    vk::ImageCreateInfo image_info {};
    image_info.setImageType(type);       // Dim count
    image_info.extent.setWidth(width);   // Image width
    image_info.extent.setHeight(height); // Image height
    image_info.extent.setDepth(depth);   // Image depth (always 1 for 2D)
    image_info.setMipLevels(mip_levels); // Maximum number of mipmaping levels
    image_info.setArrayLayers(array_layers); // Number of image layers
    image_info.setFormat(format);            // Image format used
    image_info.setTiling(tiling); // Image tiling used (Liner or Optimal)
    // Either Undefined or Preinitialized. Determins whether the firs transition
    // will preserve the texels
    image_info.setInitialLayout(vk::ImageLayout::eUndefined);
    image_info.setUsage(usage); // Specifies purpose of the image data
    // Determins ownership between Queue families (Exclusive or Concurrent)
    image_info.setSharingMode(vk::SharingMode::eExclusive);
    image_info.setSamples(number_of_samples); // Number of samples used for MSAA
    image_info.setFlags(flags);               // Image creation flags

    try {
        _handle = _device->handle().createImage(image_info, _allocator);
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // Allocate image memory
    auto memory_requirements =
        _device->handle().getImageMemoryRequirements(handle);

    vk::MemoryAllocateInfo allocation_info {};
    // Number of bytes to be allocated
    allocation_info.setAllocationSize(memory_requirements.size);
    // Type of memory we wish to allocate from
    auto memory_type = _device->find_memory_type(
        memory_requirements.memoryTypeBits, properties
    );
    if (memory_type.has_error())
        Logger::fatal(RENDERER_VULKAN_LOG, memory_type.error().what());
    allocation_info.setMemoryTypeIndex(memory_type.value());

    try {
        _memory = _device->handle().allocateMemory(allocation_info, _allocator);
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // Bind memory to the created image
    _device->handle().bindImageMemory(handle, memory, 0);
}

} // namespace ENGINE_NAMESPACE