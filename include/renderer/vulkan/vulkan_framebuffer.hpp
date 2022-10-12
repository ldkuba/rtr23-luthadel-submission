#pragma once

#include "property.hpp"
#include "logger.hpp"
#include <vulkan/vulkan.hpp>

class VulkanRenderPass;

class VulkanFramebuffer {
  public:
    Property<vk::Framebuffer> handle {
        Get { return _handle; }
    };

    VulkanFramebuffer(
        const vk::Device* const              device,
        const vk::AllocationCallbacks* const allocator,
        const VulkanRenderPass* const        render_pass,
        const uint32                         width,
        const uint32                         height,
        const std::vector<vk::ImageView>     attachments
    );
    ~VulkanFramebuffer();

    void recreate(
        const uint32                     width,
        const uint32                     height,
        const std::vector<vk::ImageView> attachments
    );

  private:
    const vk::Device*                    _device;
    const vk::AllocationCallbacks* const _allocator;

    vk::Framebuffer         _handle = {};
    const VulkanRenderPass* _render_pass;

    void create(
        const uint32                     width,
        const uint32                     height,
        const std::vector<vk::ImageView> attachments
    );
};
