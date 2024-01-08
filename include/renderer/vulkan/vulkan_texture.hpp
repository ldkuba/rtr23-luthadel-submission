#pragma once

#include "vulkan_command_pool.hpp"
#include "resources/texture.hpp"
#include "vulkan_image.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Vulkan implementation of generic texture class.
 */
class VulkanTexture : public Texture {
  public:
    /// @brief Vulkan image object connected with this texture instance
    Property<VulkanImage*> image {
        GET { return _image; }
        SET { _image = value; }
    };

    /**
     * @brief Vulkan implementation of generic texture map class.
     */
    struct Map : public Texture::Map {
        vk::Sampler sampler;

        Map(const Texture::Map::Config& config, const vk::Sampler& sampler)
            : Texture::Map(config), sampler(sampler) {}
    };

  public:
    /**
     * @brief Construct a new Vulkan Texture object
     * @param config Texture configuration
     * @param image Vulkan image which holds GPU data of this texture
     * @param command_pool Vulkan command pool reference
     * @param command_buffer Main vulkan command buffer used for frame rendering
     * @param device Vulkan device reference
     * @param allocator Allocator used
     */
    VulkanTexture(
        const Config&                        config,
        VulkanImage* const                   image,
        const VulkanCommandPool* const       command_pool,
        const VulkanCommandBuffer* const     command_buffer,
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator
    );
    virtual ~VulkanTexture();

    Outcome write(
        const byte* const data, const uint32 size, const uint32 offset
    ) override;
    Outcome resize(const uint32 width, const uint32 height) override;

    Outcome transition_render_target(const uint64 frame_number) override;

    vk::Format get_vulkan_format() const;

    static vk::Format parse_format_for_vulkan(
        const Format format, const uint32 channel_count
    );
    static Format parse_format_from_vulkan(const vk::Format format);

  private:
    VulkanImage*                         _image;
    const VulkanCommandPool* const       _command_pool;
    const VulkanCommandBuffer* const     _command_buffer;
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;
};

} // namespace ENGINE_NAMESPACE