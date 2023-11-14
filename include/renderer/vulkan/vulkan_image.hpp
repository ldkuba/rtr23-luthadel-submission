#pragma once

#include "vulkan_device.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Vulkan implementation of an on-device image.
 */
class VulkanImage {
  public:
    /// @brief Handle to the vk::Image
    Property<vk::Image> handle {
        GET { return _handle; }
    };
    /// @brief Pointer to on device memory the allocated image
    Property<vk::DeviceMemory> memory {
        GET { return _memory; }
    };
    /// @brief Image view
    Property<vk::ImageView> view {
        GET { return _view; }
    };

    /// @brief Image width
    Property<uint32> width {
        GET { return _width; }
    };
    /// @brief Image height
    Property<uint32> height {
        GET { return _height; }
    };
    /// @brief Number of mipmap levels used
    Property<uint8> mip_levels {
        GET { return _mip_levels; }
    };

    /**
     * @brief Construct a new Vulkan Image object
     *
     * @param device Vulkan device reference
     * @param allocator Allocation callback used
     */
    VulkanImage(
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator
    )
        : _device(device), _allocator(allocator) {}
    ~VulkanImage();

    /// @brief Creates vulkan image object based on a preexisting vulkan image.
    /// Usually used by internal components like swapchain.
    /// @param width Image width in pixels
    /// @param height Image height in pixels
    void create(
        const vk::Image handle, const uint32 width, const uint32 height
    );

    /// @brief Creates and allocates vulkan image in device local memory
    /// @param width Image width
    /// @param height Image height
    /// @param mip_levels Max number mipmaping levels
    /// @param number_of_samples Number of MSAA samples used
    /// @param format Image format
    /// @param tiling Image tiling
    /// @param usage Purpose of the image (Allow for better driver
    /// optimizations)
    /// @param properties Properties of the allocated memory
    void create(
        const uint32                  width,
        const uint32                  height,
        const uint8                   mip_levels,
        const vk::SampleCountFlagBits number_of_samples,
        const vk::Format              format,
        const vk::ImageTiling         tiling,
        const vk::ImageUsageFlags     usage,
        const vk::MemoryPropertyFlags properties
    );

    /// @brief Creates and allocates vulkan image in device local memory.
    /// Additionally creates an appropriate image view
    /// @param width Image width
    /// @param height Image height
    /// @param mip_levels Max number mipmaping levels
    /// @param number_of_samples Number of MSAA samples used
    /// @param format Image format
    /// @param tiling Image tiling
    /// @param usage Purpose of the image (Allow for better driver
    /// optimizations)
    /// @param properties Properties of the allocated memory
    /// @param aspect_flags Image aspect covered (eg. color, depth...)
    void create(
        const uint32                  width,
        const uint32                  height,
        const uint8                   mip_levels,
        const vk::SampleCountFlagBits number_of_samples,
        const vk::Format              format,
        const vk::ImageTiling         tiling,
        const vk::ImageUsageFlags     usage,
        const vk::MemoryPropertyFlags properties,
        const vk::ImageAspectFlags    aspect_flags
    );

    /// @brief Create image view for an already created handel.
    /// @param image Image handle
    /// @param mip_levels Max number mipmaping levels
    /// @param format Image format
    /// @param aspect_flags Image aspect covered (eg. color, depth...)
    void create(
        const vk::Image            image,
        const uint8                mip_levels,
        const vk::Format           format,
        const vk::ImageAspectFlags aspect_flags
    );

    /// @brief Creates and set an image view for current image
    /// @param mip_levels Max number mipmaping levels
    /// @param format Image format
    /// @param aspect_flags Image aspect covered (eg. color, depth...)
    /// @returns Image view
    void create_view(
        const uint8                mip_levels,
        const vk::Format           format,
        const vk::ImageAspectFlags aspect_flags
    );

    /// @brief Destroy currently set image view if it exists. Otherwise does
    /// nothing.
    void destroy_view();

    /// @brief Transition image between layouts
    /// @param command_buffer Command buffer to witch the transition command
    /// will be submitted
    /// @param old_layout Currently active image layout
    /// @param new_layout Image layout to transition to
    /// @throws InvalidArgument Exception if invalid layout transition is
    /// provided
    Result<void, InvalidArgument> transition_image_layout(
        const vk::CommandBuffer& command_buffer,
        const vk::ImageLayout    old_layout,
        const vk::ImageLayout    new_layout
    ) const;

    /// @brief Generate image mipmap levels
    /// @param command_buffer Command buffer to witch the generation command
    /// will be submitted
    void generate_mipmaps(const vk::CommandBuffer& command_buffer) const;

  private:
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::Image        _handle;
    vk::DeviceMemory _memory;
    vk::ImageView    _view;
    bool             _has_view = false;

    uint32               _width;
    uint32               _height;
    uint8                _mip_levels;
    vk::Format           _format;
    vk::ImageAspectFlags _aspect_flags;
};

} // namespace ENGINE_NAMESPACE