#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>
#include <functional>

#include "string.hpp"
#include "math_libs.hpp"
#include "vector.hpp"
#include "set.hpp"
#include "resources/texture.hpp"

namespace ENGINE_NAMESPACE {

#define RENDERER_VULKAN_LOG "Renderer :: VULKAN :: "

/**
 * @brief Indices of all vulkan queue families. Initially all unset.
 */
struct QueueFamilyIndices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> compute_family;
    std::optional<uint32> transfer_family;
    std::optional<uint32> present_family;

    bool        is_complete() const;
    Set<uint32> get_unique_indices() const;
};

/**
 * @brief Packet containing all relevant swapchain support details
 */
struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR   capabilities;
    Vector<vk::SurfaceFormatKHR> formats;
    Vector<vk::PresentModeKHR>   presentation_modes;

    vk::Extent2D get_extent(const uint32 width, const uint32 height) const;
    vk::SurfaceFormatKHR get_surface_format() const;
    vk::PresentModeKHR   get_presentation_mode() const;
};

/**
 * @brief Physical device info packet.
 */
struct PhysicalDeviceInfo {
    String               name;
    String               type;
    String               driver_version;
    String               api_version;
    float32              max_sampler_anisotropy;
    vk::SampleCountFlags framebuffer_color_sample_counts;
    vk::SampleCountFlags framebuffer_depth_sample_counts;
    uint32               min_ubo_alignment;

    Vector<float32>        memory_size_in_gb;
    Vector<vk::MemoryType> memory_types;
    Vector<bool>           memory_is_local;
    bool                   supports_device_local_host_visible_memory = false;

    // Swapchain
    std::function<SwapchainSupportDetails(const vk::SurfaceKHR&)>
        get_swapchain_support_details;

    // Format properties
    std::function<vk::FormatProperties(const vk::Format)> get_format_properties;
};

/**
 * @brief Vulkan command buffer. Spawned and managed by the parent Vulkan
 * command pool.
 */
struct VulkanCommandBuffer {
    /// @brief Currently recorded frame
    uint8                    current_frame;
    /// @brief Handle to the currently recorded buffer
    const vk::CommandBuffer* handle;

    VulkanCommandBuffer(const Vector<vk::CommandBuffer>& buffers);
    VulkanCommandBuffer(Vector<vk::CommandBuffer>&& buffers);

    /**
     * @brief Flushes contents and resets recording
     * @param current_frame Index of the next frame
     */
    void reset(const uint8 current_frame);

  private:
    const Vector<vk::CommandBuffer> _buffers;
};

/**
 * @brief Vulkan geometry data packet
 */
struct VulkanGeometryData {
    uint32 vertex_count;
    uint32 vertex_size;
    uint32 vertex_offset;
    uint32 index_count;
    uint32 index_size;
    uint32 index_offset;
};

} // namespace ENGINE_NAMESPACE