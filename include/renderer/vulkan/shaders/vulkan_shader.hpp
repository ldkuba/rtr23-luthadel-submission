#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "defines.hpp"

class VulkanShader {
private:
    vk::Device* _device;
    vk::AllocationCallbacks* _allocator;

public:
    VulkanShader(vk::Device* device, vk::AllocationCallbacks* allocator);
    ~VulkanShader();

    vk::ShaderModule create_shader_module(std::vector<byte> code);
};
