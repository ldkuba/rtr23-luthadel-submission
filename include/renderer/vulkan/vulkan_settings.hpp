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
        // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
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
    constexpr static vk::PhysicalDeviceFeatures required_device_features = {
        false,  // robustBufferAccess
        false,  // fullDrawIndexUint32
        false,  // imageCubeArray
        false,  // independentBlend
        false,  // geometryShader
        false,  // tessellationShader
        false,  // sampleRateShading
        false,  // dualSrcBlend
        false,  // logicOp
        false,  // multiDrawIndirect
        false,  // drawIndirectFirstInstance
        false,  // depthClamp
        false,  // depthBiasClamp
        false,  // fillModeNonSolid
        false,  // depthBounds
        false,  // wideLines
        false,  // largePoints
        false,  // alphaToOne
        false,  // multiViewport
        true,   // samplerAnisotropy
        false,  // textureCompressionETC2
        false,  // textureCompressionASTC_LDR
        false,  // textureCompressionBC
        false,  // occlusionQueryPrecise
        false,  // pipelineStatisticsQuery
        false,  // vertexPipelineStoresAndAtomics
        false,  // fragmentStoresAndAtomics
        false,  // shaderTessellationAndGeometryPointSize
        false,  // shaderImageGatherExtended
        false,  // shaderStorageImageExtendedFormats
        false,  // shaderStorageImageMultisample
        false,  // shaderStorageImageReadWithoutFormat
        false,  // shaderStorageImageWriteWithoutFormat
        false,  // shaderUniformBufferArrayDynamicIndexing
        false,  // shaderSampledImageArrayDynamicIndexing
        false,  // shaderStorageBufferArrayDynamicIndexing
        false,  // shaderStorageImageArrayDynamicIndexing
        false,  // shaderClipDistance
        false,  // shaderCullDistance
        false,  // shaderFloat64
        false,  // shaderInt64
        false,  // shaderInt16
        false,  // shaderResourceResidency
        false,  // shaderResourceMinLod
        false,  // sparseBinding
        false,  // sparseResidencyBuffer
        false,  // sparseResidencyImage2D
        false,  // sparseResidencyImage3D
        false,  // sparseResidency2Samples
        false,  // sparseResidency4Samples
        false,  // sparseResidency8Samples
        false,  // sparseResidency16Samples
        false,  // sparseResidencyAliased
        false,  // variableMultisampleRate
        false   // inheritedQueries
    };

    // Device suitability scores
    constexpr static int32 base_score = 1;
    constexpr static int32 discrete_gpu_score = 1000;
    constexpr static float32 max_texture_size_weight = 1.0f;

    // Device required extensions
    static const std::vector<const char*> device_required_extensions;

    // Swapchain
    constexpr static vk::Format preferred_swapchain_format = vk::Format::eB8G8R8A8Srgb;
    constexpr static vk::ColorSpaceKHR preferred_swapchain_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
    constexpr static vk::PresentModeKHR preferred_swapchain_presentation_mode = vk::PresentModeKHR::eMailbox;
};