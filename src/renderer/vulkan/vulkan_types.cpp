#include "renderer/vulkan/vulkan_types.hpp"
#include "renderer/vulkan/vulkan_settings.hpp"

bool QueueFamilyIndices::is_complete() const {
    return
        (!VulkanSettings::graphics_family_required || graphics_family.has_value()) &&
        (!VulkanSettings::compute__family_required || compute_family.has_value()) &&
        (!VulkanSettings::transfer_family_required || transfer_family.has_value()) &&
        (!VulkanSettings::present__family_required || present_family.has_value());
}
std::set<uint32> QueueFamilyIndices::get_unique_indices() const {
    std::set<uint32> unique_indices = {
        graphics_family.value_or(-1),
        compute_family.value_or(-1),
        transfer_family.value_or(-1),
        present_family.value_or(-1)
    };
    unique_indices.erase(-1);
    return unique_indices;
}

vk::Extent2D SwapchainSupportDetails::get_extent(const uint32 width, const uint32 height) const {
    // Return required width and height if supported
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    return {
        std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}
vk::SurfaceFormatKHR SwapchainSupportDetails::get_surface_format() const {
    // Return preferred format if supported, otherwise return first supported format
    for (auto format : formats) {
        if (format.format == VulkanSettings::preferred_swapchain_format &&
            format.colorSpace == VulkanSettings::preferred_swapchain_color_space)
            return format;
    }
    return formats[0];
}
vk::PresentModeKHR SwapchainSupportDetails::get_presentation_mode() const {
    // Return preferred presentation mode if supported, otherwise return FIFO
    for (const auto& presentation_mode : presentation_modes) {
        if (presentation_mode == VulkanSettings::preferred_swapchain_presentation_mode)
            return presentation_mode;
    }
    return vk::PresentModeKHR::eFifo;
}