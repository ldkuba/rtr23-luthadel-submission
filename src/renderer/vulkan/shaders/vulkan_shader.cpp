#include "renderer/vulkan/shaders/vulkan_shader.hpp"

VulkanShader::VulkanShader(vk::Device* device, vk::AllocationCallbacks* allocator)
    : _device(device), _allocator(allocator) {}
VulkanShader::~VulkanShader() {}

vk::ShaderModule VulkanShader::create_shader_module(std::vector<byte> code) {
    vk::ShaderModuleCreateInfo create_info{};
    create_info.setCodeSize(code.size());
    create_info.setPCode(reinterpret_cast<const uint32*> (code.data()));
    return _device->createShaderModule(create_info, _allocator);
}