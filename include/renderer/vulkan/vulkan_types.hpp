#pragma once

#define RENDERER_VULKAN_LOG "Renderer :: VULKAN :: "

#include <vulkan/vulkan.hpp>
#include <optional>
#include <set>
#include <functional>

#include "defines.hpp"

struct QueueFamilyIndices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> compute_family;
    std::optional<uint32> transfer_family;
    std::optional<uint32> present_family;

    bool is_complete() const;
    std::set<uint32> get_unique_indices() const;
};

struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentation_modes;

    vk::Extent2D get_extent(const uint32 width, const uint32 height) const;
    vk::SurfaceFormatKHR get_surface_format() const;
    vk::PresentModeKHR get_presentation_mode() const;
};

struct PhysicalDeviceInfo {
    std::string name;
    std::string type;
    std::string driver_version;
    std::string api_version;
    float32 max_sampler_anisotropy;
    vk::SampleCountFlags framebuffer_color_sample_counts;
    vk::SampleCountFlags framebuffer_depth_sample_counts;

    std::vector<float32> memory_size_in_gb;
    std::vector<vk::MemoryType> memory_types;
    std::vector<bool> memory_is_local;

    // Swapchain
    std::function<SwapchainSupportDetails(const vk::SurfaceKHR&)>get_swapchain_support_details;

    // Format properties
    std::function<vk::FormatProperties(const vk::Format)> get_format_properties;
};

struct DescriptorInfo {
    vk::DescriptorType type;
    vk::ShaderStageFlagBits shader_stage;
    uint32 count;
};

#include "resources/texture.hpp"

class VulkanImage;

struct VulkanTextureData : public InternalTextureData {
    VulkanImage* image;
    vk::Sampler sampler;
};

struct VulkanGeometryData {
    uint32 vertex_count;
    uint32 vertex_size;
    uint32 vertex_offset;
    uint32 index_count;
    uint32 index_size;
    uint32 index_offset;
};

