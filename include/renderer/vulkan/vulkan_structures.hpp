#pragma once

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

    bool is_complete();
    std::set<uint32> get_unique_indices();
};

struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentation_modes;

    vk::Extent2D get_extent(uint32 width, uint32 height);
    vk::SurfaceFormatKHR get_surface_format();
    vk::PresentModeKHR get_presentation_mode();
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
    std::function<SwapchainSupportDetails(vk::SurfaceKHR)>get_swapchain_support_details;

    // Format properties
    std::function<vk::FormatProperties(vk::Format)> get_format_properties;
};