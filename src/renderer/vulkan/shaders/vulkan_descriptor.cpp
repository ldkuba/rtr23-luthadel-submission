#include "renderer/vulkan/shaders/vulkan_descriptor.hpp"

// Constructor & Destructor
VulkanDescriptor::VulkanDescriptor(
    const VulkanDevice* const device,
    const vk::AllocationCallbacks* const allocator
) : _device(device), _allocator(allocator) {}
VulkanDescriptor::~VulkanDescriptor() {
    for (uint32 i = 0; i < _descriptor_infos.size(); i++) {
        if (_descriptors[i] == nullptr) continue;
        switch (_descriptor_infos[i].type) {
        case vk::DescriptorType::eUniformBuffer:
        {
            auto buffer = (BufferDescriptor*) _descriptors[i];
            delete buffer;
        }
        break;
        case vk::DescriptorType::eCombinedImageSampler:
        {
            auto sampler = (SamplerDescriptor*) _descriptors[i];
            delete sampler;
        }
        default:
            delete _descriptors[i];
            break;
        }
    }
    if (_pool)
        _device->handle().destroyDescriptorPool(_pool, _allocator);
    if (_set_layout)
        _device->handle().destroyDescriptorSetLayout(_set_layout, _allocator);
}

// //// //////////////////////////// //
// VULKAN DESCRIPTOR PUBLIC METHODS //
// //////////////////////////////// //

void VulkanDescriptor::add_uniform_buffer(
    const vk::ShaderStageFlagBits shader_stage,
    const uint32 count,
    const vk::DeviceSize buffer_size
) {
    // Remember descriptor info
    DescriptorInfo info = {
        vk::DescriptorType::eUniformBuffer,
        shader_stage,
        count
    };
    _descriptor_infos.push_back(info);

    // Create uniform buffer
    std::array<VulkanBuffer*, VulkanSettings::max_frames_in_flight> uniform_buffer = {};
    for (uint32 i = 0; i < VulkanSettings::max_frames_in_flight; i++) {
        uniform_buffer[i] = new VulkanBuffer(_device, _allocator);
        uniform_buffer[i]->create(
            buffer_size,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent
        );
    }

    // Create actual descriptor
    Descriptor* descriptor = new BufferDescriptor(uniform_buffer);
    _descriptors.push_back(descriptor);
}

void VulkanDescriptor::add_image_sampler(
    const vk::ShaderStageFlagBits shader_stage,
    const uint32 count) {
    // Remember descriptor info
    DescriptorInfo info = {
        vk::DescriptorType::eCombinedImageSampler,
        shader_stage,
        count
    };
    _descriptor_infos.push_back(info);
}

void VulkanDescriptor::create_bindings(
    const uint32 max_sets,
    const bool can_free
) {
    // Create descriptor set layout
    std::vector<vk::DescriptorSetLayoutBinding> bindings(_descriptor_infos.size());
    for (uint32 i = 0; i < _descriptor_infos.size(); i++) {
        bindings[i].setBinding(i);
        bindings[i].setDescriptorCount(1);
        bindings[i].setDescriptorType(_descriptor_infos[i].type);
        bindings[i].setStageFlags(_descriptor_infos[i].shader_stage);
        bindings[i].setPImmutableSamplers(nullptr);
    }

    vk::DescriptorSetLayoutCreateInfo layout_info{};
    layout_info.setBindings(bindings);

    try {
        _set_layout = _device->handle().createDescriptorSetLayout(layout_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(RENDERER_VULKAN_LOG, e.what()); }

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> pool_sizes(_descriptor_infos.size());
    for (uint32 i = 0; i < _descriptor_infos.size(); i++) {
        pool_sizes[i].setType(_descriptor_infos[i].type);
        pool_sizes[i].setDescriptorCount(_descriptor_infos[i].count);
    }

    vk::DescriptorPoolCreateInfo create_info{};
    create_info.setPoolSizes(pool_sizes);
    create_info.setMaxSets(max_sets);
    if (can_free) // Allows explicit call of free commands on descriptor pools
        create_info.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    try {
        _pool = _device->handle().createDescriptorPool(create_info, _allocator);
    } catch (vk::SystemError e) { Logger::fatal(RENDERER_VULKAN_LOG, e.what()); }
}

VulkanBuffer* VulkanDescriptor::get_buffer(const uint32 binding, const uint32 frame) const {
    if (_descriptor_infos[binding].type != vk::DescriptorType::eUniformBuffer) {
        Logger::error(RENDERER_VULKAN_LOG, "There is no buffer at binding ", binding,
            ". Value of nullptr returned.");
        return nullptr;
    }
    auto buffer_descriptor = (BufferDescriptor*) _descriptors[binding];
    return buffer_descriptor->buffers[frame];
}
