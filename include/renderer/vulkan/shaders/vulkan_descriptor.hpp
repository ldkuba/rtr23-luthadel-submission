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
            std::array<VulkanBuffer*, VulkanSettings::max_frames_in_flight>
                buffers
        )
            : buffers(buffers) {}
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
        vk::DescriptorType      type;
        vk::ShaderStageFlagBits shader_stage;
        uint32                  count;
    };

  public:
    /// @brief vk::DescriptorPool instance
    Property<vk::DescriptorPool> pool {
        Get { return _pool; }
    };
    /// @brief vk::DescriptorSetLayout instance
    Property<vk::DescriptorSetLayout> set_layout {
        Get { return _set_layout; }
    };

    VulkanDescriptor(
        const VulkanDevice* const            device,
        const vk::AllocationCallbacks* const allocator
    );
    ~VulkanDescriptor();

    /// @brief Adds a uniform buffer binding
    /// @param shader_stage Stages which will use this binding
    /// @param count Total descriptor count of this type that can be allocated.
    /// Note: This maximum applies across all sets
    /// @param buffer_size Total uniform buffer size in bytes
    /// @returns Descriptors binding index
    uint32 add_uniform_buffer(
        const vk::ShaderStageFlagBits shader_stage,
        const uint32                  count,
        const vk::DeviceSize          buffer_size
    );
    /// @brief Adds a Combined image sampler binding
    /// @param shader_stage Stages which will use this binding
    /// @param count Total descriptor count of this type that can be allocated.
    /// Note: This maximum applies across all sets
    /// @returns Descriptors binding index
    uint32 add_image_sampler(
        const vk::ShaderStageFlagBits shader_stage, const uint32 count
    );
    /// @brief Creates descriptor pool and set layout with all the added
    /// bindings
    /// @param max_sets Maximum to the number of descriptor sets that can be
    /// allocated
    /// @param can_free Allows explicit call of free commands on descriptors if
    /// true
    void create_pool_and_layout(const uint32 max_sets, const bool can_free);

    /// @brief Get the descriptors uniform buffer
    /// @param binding Binding index of the requested uniform buffer
    /// @param index Index into the requested uniform buffer array
    /// @returns VulkanBuffer*
    VulkanBuffer* get_buffer(const uint32 binding, const uint32 index) const;

  private:
    const VulkanDevice*                  _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::DescriptorPool      _pool;
    vk::DescriptorSetLayout _set_layout;

    std::vector<DescriptorInfo> _descriptor_infos;
    std::vector<Descriptor*>    _descriptors;
};