#pragma once

#include "vulkan_command_pool.hpp"
#include "vulkan_device.hpp"

class VulkanImage {
private:
    VulkanDevice* _device;
    vk::AllocationCallbacks* _allocator;

    bool _has_view = false;
    vk::ImageAspectFlags _aspect_flags;

    void create_view(
        uint32 mip_levels,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags
    );

public:
    /// @brief Handle to the vk::Image
    vk::Image handle;
    /// @brief Pointer to the allocated image on device memory
    vk::DeviceMemory memory;
    /// @brief Image view
    vk::ImageView view;

    /// @brief Image width
    uint32 width;
    /// @brief Image height
    uint32 height;

    VulkanImage(VulkanDevice* device, vk::AllocationCallbacks* allocator) : _device(device), _allocator(allocator) {}
    ~VulkanImage();

    /// @brief Creates and allocates vulkan image in device local memory
    /// @param width Image width
    /// @param height Image height
    /// @param mip_levels Max number mipmaping levels
    /// @param number_of_samples Number of MSAA samples used
    /// @param format Image format
    /// @param tiling Image tiling
    /// @param usage Purpose of the image (Allow for better driver optimizations)
    /// @param properties Properties of the allocated memory
    void create(
        uint32 width,
        uint32 height,
        uint32 mip_levels,
        vk::SampleCountFlagBits number_of_samples,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties
    );

    /// @brief Creates and allocates vulkan image in device local memory.
    /// Additionally creates an appropriate image view
    /// @param width Image width
    /// @param height Image height
    /// @param mip_levels Max number mipmaping levels
    /// @param number_of_samples Number of MSAA samples used
    /// @param format Image format
    /// @param tiling Image tiling
    /// @param usage Purpose of the image (Allow for better driver optimizations)
    /// @param properties Properties of the allocated memory
    /// @param aspect_flags Image aspect covered (eg. color, depth...)
    void create(
        uint32 width,
        uint32 height,
        uint32 mip_levels,
        vk::SampleCountFlagBits number_of_samples,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties,
        vk::ImageAspectFlags aspect_flags
    );

    /// @brief Create image view for an already created handel.
    /// @param image Image handle
    /// @param mip_levels Max number mipmaping levels
    /// @param format Image format
    /// @param aspect_flags Image aspect covered (eg. color, depth...)
    void create(
        vk::Image image,
        uint32 mip_levels,
        vk::Format format,
        vk::ImageAspectFlags aspect_flags
    );

    /// @brief Transition image between layouts
    /// @param command_pool Command pool to witch the transition command will be submitted
    /// @param format Image format
    /// @param old_layout Currently active image layout
    /// @param new_layout Image layout to transition to
    /// @param mip_levels Max number mipmaping levels
    /// @throws std::invalid_argument Exception if invalid layout transition is provided
    void transition_image_layout(
        VulkanCommandPool* command_pool,
        vk::Format format,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        uint32 mip_levels
    );

    /// @brief Creates an image view corresponding to the provided vk::Image
    /// @param format Image format
    /// @param aspect_flags Image aspect covered (eg. color, depth...)
    /// @param image Image for which we want to create the view
    /// @param device Device on which the image is stored
    /// @param allocator Vulkan allocation callback
    /// @returns Image view
    static vk::ImageView get_view_from_image(
        vk::Format format,
        vk::ImageAspectFlags aspect_flags,
        const vk::Image& image,
        const vk::Device& device,
        const vk::AllocationCallbacks* allocator
    );
};
