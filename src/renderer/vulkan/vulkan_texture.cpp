#include "renderer/vulkan/vulkan_texture.hpp"

#include "renderer/vulkan/vulkan_buffer.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
VulkanTexture::VulkanTexture(
    const Config&                        config,
    VulkanImage* const                   image,
    const VulkanCommandPool* const       command_pool,
    const VulkanCommandBuffer* const     command_buffer,
    const VulkanDevice* const            device,
    const vk::AllocationCallbacks* const allocator
)
    : Texture(config), _image(image), _command_pool(command_pool),
      _command_buffer(command_buffer), _device(device), _allocator(allocator) {}

VulkanTexture::~VulkanTexture() {
    if (_image) del(_image);
}

// ///////////////////////////// //
// VULKAN TEXTURE PUBLIC METHODS //
// ///////////////////////////// //

Outcome VulkanTexture::write(
    const byte* const data, const uint32 size, const uint32 offset
) {
    if (Texture::write(data, size, offset).failed()) return Outcome::Failed;

    // Create staging buffer
    auto staging_buffer =
        new (MemoryTag::Temp) VulkanBuffer(_device, _allocator);
    staging_buffer->create(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Fill created memory with data
    staging_buffer->load_data(data, 0, size);

    auto command_buffer = _command_pool->begin_single_time_commands();

    // Transition image to a layout optimal for data transfer
    auto result = _image->transition_image_layout(
        command_buffer,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal
    );
    if (result.has_error()) {
        Logger::error(RENDERER_VULKAN_LOG, result.error().what());
        return Outcome::Failed;
    }

    // Copy buffer data to image
    staging_buffer->copy_data_to_image(command_buffer, _image);

    // Generate mipmaps, this also transitions image to a layout optimal for
    // sampling
    _image->generate_mipmaps(command_buffer);
    _command_pool->end_single_time_commands(command_buffer);

    // Cleanup
    del(staging_buffer);

    return Outcome::Successful;
}

Outcome VulkanTexture::resize(const uint32 width, const uint32 height) {
    if (Texture::resize(width, height).failed()) return Outcome::Failed;
    if (is_wrapped()) return Outcome::Successful;

    // Destroy old image
    del(_image);

    // Get format
    const auto texture_format = get_vulkan_format();

    // Get usage flags
    vk::ImageUsageFlagBits  usage_flags;
    vk::ImageAspectFlagBits aspect_flags;
    if (Texture::has_depth_format(_format)) {
        usage_flags  = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        aspect_flags = vk::ImageAspectFlagBits::eDepth;
    } else {
        usage_flags  = vk::ImageUsageFlagBits::eColorAttachment;
        aspect_flags = vk::ImageAspectFlagBits::eColor;
    }

    // Create new image
    auto texture_image =
        new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    texture_image->create_2d(
        width,
        height,
        mip_level_count,
        vk::SampleCountFlagBits::e1,
        texture_format,
        vk::ImageTiling::eOptimal,
        is_render_target() ? usage_flags | vk::ImageUsageFlagBits::eSampled
                           : vk::ImageUsageFlagBits::eTransferSrc |
                                 vk::ImageUsageFlagBits::eTransferDst |
                                 vk::ImageUsageFlagBits::eSampled | usage_flags,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        aspect_flags
    );

    // Assign
    _image = texture_image;

    return Outcome::Successful;
}

Outcome VulkanTexture::transition_render_target() const {
    if (!is_render_target()) {
        Logger::error(
            RENDERER_VULKAN_LOG,
            "Texture transition for render target texture attempted, but given "
            "texture isn't marked as render target. Operation failed."
        );
        return Outcome::Failed;
    }

    const auto depth_format = has_depth_format(_format);
    const auto from =
        used_in_render_pass()
            ? depth_format ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                           : vk::ImageLayout::eColorAttachmentOptimal
            : vk::ImageLayout::eUndefined;
    const auto to = depth_format ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                 : vk::ImageLayout::eShaderReadOnlyOptimal;

    const auto res = _image->transition_image_layout( //
        *_command_buffer->handle,
        from,
        to
    );
    if (res.has_error()) {
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Texture transition from attachment to readable texture failed. ",
            res.error().what()
        );
        return Outcome::Failed;
    }
    return Outcome::Successful;
}

vk::Format VulkanTexture::get_vulkan_format() const {
    return parse_format_for_vulkan(_format, _channel_count);
}

vk::Format VulkanTexture::parse_format_for_vulkan(
    const Format format, const uint32 channel_count
) {
    switch (format) {
    case Format::RGBA8Unorm:
        switch (channel_count) {
        case 1: return vk::Format::eR8Unorm;
        case 2: return vk::Format::eR8G8Unorm;
        case 3: return vk::Format::eR8G8B8Unorm;
        case 4: return vk::Format::eR8G8B8A8Unorm;
        }
    case Format::RGBA8Srgb:
        switch (channel_count) {
        case 1: return vk::Format::eR8Srgb;
        case 2: return vk::Format::eR8G8Srgb;
        case 3: return vk::Format::eR8G8B8Srgb;
        case 4: return vk::Format::eR8G8B8A8Srgb;
        }
    case Format::RGBA16Unorm:
        switch (channel_count) {
        case 1: return vk::Format::eR16Unorm;
        case 2: return vk::Format::eR16G16Unorm;
        case 3: return vk::Format::eR16G16B16Unorm;
        case 4: return vk::Format::eR16G16B16A16Unorm;
        }
    case Format::RGBA16Sfloat:
        switch (channel_count) {
        case 1: return vk::Format::eR16Sfloat;
        case 2: return vk::Format::eR16G16Sfloat;
        case 3: return vk::Format::eR16G16B16Sfloat;
        case 4: return vk::Format::eR16G16B16A16Sfloat;
        }
    case Format::RGBA32Sfloat:
        switch (channel_count) {
        case 1: return vk::Format::eR32Sfloat;
        case 2: return vk::Format::eR32G32Sfloat;
        case 3: return vk::Format::eR32G32B32Sfloat;
        case 4: return vk::Format::eR32G32B32A32Sfloat;
        }
    case Format::BGRA8Unorm:
        switch (channel_count) {
        case 1: return vk::Format::eR8Unorm;
        case 2: return vk::Format::eR8G8Unorm;
        case 3: return vk::Format::eB8G8R8Unorm;
        case 4: return vk::Format::eB8G8R8A8Unorm;
        }
    case Format::BGRA8Srgb:
        switch (channel_count) {
        case 1: return vk::Format::eR8Srgb;
        case 2: return vk::Format::eR8G8Srgb;
        case 3: return vk::Format::eB8G8R8Srgb;
        case 4: return vk::Format::eB8G8R8A8Srgb;
        }
    case Format::D32:
        if (channel_count != 4) Logger::fatal(RENDERER_VULKAN_LOG, "");
        return vk::Format::eD32Sfloat;
    case Format::DS32:
        if (channel_count != 4) Logger::fatal(RENDERER_VULKAN_LOG, "");
        return vk::Format::eD32SfloatS8Uint;
    case Format::DS24:
        if (channel_count != 3) Logger::fatal(RENDERER_VULKAN_LOG, "");
        return vk::Format::eD24UnormS8Uint;
    default:
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Unimplemented texture format conversion requested."
        );
    }
    return {};
}

Texture::Format VulkanTexture::parse_format_from_vulkan( //
    const vk::Format format
) {
    switch (format) {
    case vk::Format::eR8Unorm:
    case vk::Format::eR8G8Unorm:
    case vk::Format::eR8G8B8Unorm:
    case vk::Format::eR8G8B8A8Unorm: return Format::RGBA8Unorm;
    case vk::Format::eR8Srgb:
    case vk::Format::eR8G8Srgb:
    case vk::Format::eR8G8B8Srgb:
    case vk::Format::eR8G8B8A8Srgb: return Format::RGBA8Srgb;
    case vk::Format::eR16Unorm:
    case vk::Format::eR16G16Unorm:
    case vk::Format::eR16G16B16Unorm:
    case vk::Format::eR16G16B16A16Unorm: return Format::RGBA16Unorm;
    case vk::Format::eR16Sfloat:
    case vk::Format::eR16G16Sfloat:
    case vk::Format::eR16G16B16Sfloat:
    case vk::Format::eR16G16B16A16Sfloat: return Format::RGBA16Sfloat;
    case vk::Format::eR32Sfloat:
    case vk::Format::eR32G32Sfloat:
    case vk::Format::eR32G32B32Sfloat:
    case vk::Format::eR32G32B32A32Sfloat: return Format::RGBA32Sfloat;
    case vk::Format::eB8G8R8Unorm:
    case vk::Format::eB8G8R8A8Unorm: return Format::BGRA8Unorm;
    case vk::Format::eB8G8R8Srgb:
    case vk::Format::eB8G8R8A8Srgb: return Format::BGRA8Srgb;
    case vk::Format::eD32Sfloat: return Format::D32;
    case vk::Format::eD32SfloatS8Uint: return Format::DS32;
    case vk::Format::eD24UnormS8Uint: return Format::DS24;
    default:
        Logger::fatal(
            RENDERER_VULKAN_LOG,
            "Unimplemented vulkan format conversion requested."
        );
    }
    return {};
}

} // namespace ENGINE_NAMESPACE