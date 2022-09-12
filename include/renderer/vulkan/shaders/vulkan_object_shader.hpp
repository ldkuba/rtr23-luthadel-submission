#pragma once

#include "vulkan_shader.hpp"

class VulkanObjectShader : public VulkanShader {
private:

public:
    VulkanObjectShader(vk::Device* device, vk::AllocationCallbacks* allocator);
    ~VulkanObjectShader();
};

VulkanObjectShader::VulkanObjectShader(vk::Device* device, vk::AllocationCallbacks* allocator)
    : VulkanShader(device, allocator) {}

VulkanObjectShader::~VulkanObjectShader() {}
