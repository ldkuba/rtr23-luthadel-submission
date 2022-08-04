#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "defines.hpp"

class VulkanSettings {
public:
    // General settings
    constexpr static uint32 vulkan_version = VK_API_VERSION_1_2;

    // Validation
#ifdef NDEBUG
    constexpr static bool enable_validation_layers = false;
#else
    constexpr static bool enable_validation_layers = true;
#endif

    // Debugger
    constexpr static vk::DebugUtilsMessageSeverityFlagsEXT enabled_message_security_levels =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
    constexpr static vk::DebugUtilsMessageTypeFlagsEXT enabled_message_types =
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

    // Device requirements
    constexpr static bool graphics_family_required = true;
    constexpr static bool compute__family_required = true;
    constexpr static bool transfer_family_required = true;
    constexpr static bool present__family_required = true;

    // Device suitability scores
    constexpr static int32 base_score = 1;
    constexpr static int32 discrete_gpu_score = 1000;
    constexpr static float32 max_texture_size_weight = 1.0f;

    // Device required extensions
    static const std::vector<const char*> device_required_extensions;
};