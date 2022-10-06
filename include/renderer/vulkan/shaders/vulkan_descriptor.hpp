#ifndef __VULKAN_DESCRIPTOR_H__
#define __VULKAN_DESCRIPTOR_H__

#pragma once

#include "../vulkan_settings.hpp"
#include "../vulkan_buffer.hpp"


class VulkanDescriptor {
private:
    class Descriptor {
    public:
        Descriptor() {}
        ~Descriptor() {}
    };

    class BufferDescriptor : public Descriptor {
    public:
        std::array<VulkanBuffer*, VulkanSettings::max_frames_in_flight> buffers;

        BufferDescriptor(
            std::array<VulkanBuffer*, VulkanSettings::max_frames_in_flight> buffers
        ) : buffers(buffers) {}
        ~BufferDescriptor() {
            for (auto buffer : buffers)
                delete buffer;
        }
    };

    class SamplerDescriptor : public Descriptor {
    public:
        SamplerDescriptor() {}
        ~SamplerDescriptor() {}
    };

    struct DescriptorInfo {
        vk::DescriptorType type;
        vk::ShaderStageFlagBits shader_stage;
        uint32 count;
    };

public:
    Property<vk::DescriptorPool> pool{ Get { return _pool; } };
    Property<vk::DescriptorSetLayout> set_layout{ Get { return _set_layout; } };

    VulkanDescriptor(
        const VulkanDevice* const device,
        const vk::AllocationCallbacks* const allocator
    );
    ~VulkanDescriptor();

    void add_uniform_buffer(
        const vk::ShaderStageFlagBits shader_stage,
        const uint32 count,
        const vk::DeviceSize buffer_size
    );
    void add_image_sampler(
        const vk::ShaderStageFlagBits shader_stage,
        const uint32 count
    );
    void create_bindings(
        const uint32 max_sets,
        const bool can_free
    );

    VulkanBuffer* get_buffer(const uint32 binding, const uint32 frame) const;

private:
    const VulkanDevice* _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::DescriptorPool _pool;
    vk::DescriptorSetLayout _set_layout;

    std::vector<DescriptorInfo> _descriptor_infos;
    std::vector<Descriptor*> _descriptors;
};

#endif // __VULKAN_DESCRIPTOR_H__