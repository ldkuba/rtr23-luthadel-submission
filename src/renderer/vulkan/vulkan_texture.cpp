#include "renderer/vulkan/vulkan_texture.hpp"

#include "renderer/vulkan/vulkan_buffer.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
VulkanTexture::VulkanTexture(
    const Config&                        config,
    VulkanImage* const                   image,
    const VulkanCommandPool* const       command_pool,
    const VulkanDevice* const            device,
    const vk::AllocationCallbacks* const allocator
)
    : Texture(config), _image(image), _command_pool(command_pool),
      _device(device), _allocator(allocator) {}

VulkanTexture::~VulkanTexture() {
    if (image) del(_image);
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
        _total_size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
    );

    // Fill created memory with data
    staging_buffer->load_data(data, 0, _total_size);

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
    delete staging_buffer;

    return Outcome::Successful;
}

Outcome VulkanTexture::resize(const uint32 width, const uint32 height) {
    if (Texture::resize(width, height).failed()) return Outcome::Failed;
    if (is_wrapped()) return Outcome::Successful;

    // Destroy old image
    del(_image);

    // Get format
    const auto texture_format = channel_count_to_UNORM(channel_count);

    // Create new image
    auto texture_image =
        new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    texture_image->create(
        width,
        height,
        mip_level_count,
        vk::SampleCountFlagBits::e1,
        texture_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eTransferDst |
            vk::ImageUsageFlagBits::eSampled |
            vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );

    // Assign
    _image = texture_image;

    return Outcome::Successful;
}

vk::Format VulkanTexture::channel_count_to_SRGB(const uint8 channel_count) {
    switch (channel_count) {
    case 1: return vk::Format::eR8Srgb;
    case 2: return vk::Format::eR8G8Srgb;
    case 3: return vk::Format::eR8G8B8Srgb;
    case 4: return vk::Format::eR8G8B8A8Srgb;
    default: return vk::Format::eR8G8B8A8Srgb;
    }
}
vk::Format VulkanTexture::channel_count_to_UNORM(const uint8 channel_count) {
    switch (channel_count) {
    case 1: return vk::Format::eR8Unorm;
    case 2: return vk::Format::eR8G8Unorm;
    case 3: return vk::Format::eR8G8B8Unorm;
    case 4: return vk::Format::eR8G8B8A8Unorm;
    default: return vk::Format::eR8G8B8A8Unorm;
    }
}

} // namespace ENGINE_NAMESPACE