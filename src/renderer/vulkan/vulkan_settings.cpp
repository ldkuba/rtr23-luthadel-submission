#include "renderer/vulkan/vulkan_settings.hpp"

const std::vector<const char*> VulkanSettings::validation_layers{
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> VulkanSettings::device_required_extensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};