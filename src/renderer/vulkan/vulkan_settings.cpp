#include "renderer/vulkan/vulkan_settings.hpp"

namespace ENGINE_NAMESPACE {

const std::vector<const char*> VulkanSettings::validation_layers {
    "VK_LAYER_KHRONOS_validation"
    // Note: very verbose, enable only for debugging
    // , "VK_LAYER_LUNARG_api_dump"
};

const std::vector<const char*> VulkanSettings::device_required_extensions {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

} // namespace ENGINE_NAMESPACE