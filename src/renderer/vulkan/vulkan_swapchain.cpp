#include "renderer/vulkan/vulkan_backend.hpp"

void VulkanBackend::create_swapchain() {
    SwapchainSupportDetails swapchain_support = _physical_device_info.get_swapchain_support_details();

    vk::Extent2D extent = swapchain_support.get_extent(_surface->get_width_in_pixels(), _surface->get_height_in_pixels());
    vk::SurfaceFormatKHR surface_format = swapchain_support.get_surface_format();
    vk::PresentModeKHR presentation_mode = swapchain_support.get_presentation_mode();

    uint32 min_image_count = swapchain_support.capabilities.minImageCount + 1;
    uint32 max_image_count = swapchain_support.capabilities.maxImageCount;
    if (max_image_count != 0 && min_image_count > max_image_count)
        min_image_count = max_image_count;

    vk::SwapchainCreateInfoKHR create_info{};
    create_info.setSurface(_vulkan_surface);                                //
    create_info.setMinImageCount(min_image_count);                          //
    create_info.setImageExtent(extent);                                     //
    create_info.setImageFormat(surface_format.format);                      //
    create_info.setImageColorSpace(surface_format.colorSpace);              //
    create_info.setPresentMode(presentation_mode);                          //
    create_info.setImageArrayLayers(1);                                     //
    create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);    // TODO: postprocessing
    create_info.setPreTransform(swapchain_support.capabilities.currentTransform); //
    create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);  //
    create_info.setClipped(true);                                           //
    create_info.setOldSwapchain(VK_NULL_HANDLE);                            //

    // indices
    const uint32 queue_family_indices[] = {
        _queue_family_indices.graphics_family.value(),
        _queue_family_indices.present_family.value()
    };
    if (_queue_family_indices.graphics_family.value() != _queue_family_indices.present_family.value()) {
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
        create_info.setQueueFamilyIndexCount(2);
        create_info.setPQueueFamilyIndices(queue_family_indices);
    } else {
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        create_info.setQueueFamilyIndexCount(0);
        create_info.setPQueueFamilyIndices(nullptr);
    }

    auto result = _device.createSwapchainKHR(&create_info, _allocator, &_swapchain);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create swapchain.");

    _swapchain_images = _device.getSwapchainImagesKHR(_swapchain);
    _swapchain_format = surface_format.format;
    _swapchain_extent = extent;
}

void VulkanBackend::recreate_swapchain() {
    _device.waitIdle();

    cleanup_swapchain();

    create_swapchain();
    create_swapchain_image_views();
    create_color_resource();
    create_depth_resources();
    create_framebuffers();
}

void VulkanBackend::cleanup_swapchain() {
    // TODO: TEMP MSAA CODE
    _device.destroyImageView(_color_image_view, _allocator);
    _device.destroyImage(_color_image, _allocator);
    _device.freeMemory(_color_image_memory, _allocator);


    // TODO: TEMP DEPTH BUFFER CODE
    _device.destroyImageView(_depth_image_view, _allocator);
    _device.destroyImage(_depth_image, _allocator);
    _device.freeMemory(_depth_image_memory, _allocator);


    // TODO: TEMP FRAMEBUFFER CODE
    for (auto framebuffer : _swapchain_framebuffers)
        _device.destroyFramebuffer(framebuffer, _allocator);


    for (auto image_view : _swapchain_image_views)
        _device.destroyImageView(image_view, _allocator);

    _device.destroySwapchainKHR(_swapchain, _allocator);
}

void VulkanBackend::create_swapchain_image_views() {
    _swapchain_image_views.resize(_swapchain_images.size());

    for (uint32 i = 0; i < _swapchain_images.size(); i++) {
        _swapchain_image_views[i] = create_image_view(
            _swapchain_images[i], _swapchain_format, vk::ImageAspectFlagBits::eColor, 1
        );
    }
}

// FRAMEBUFFER
void VulkanBackend::create_framebuffers() {
    _swapchain_framebuffers.resize(_swapchain_image_views.size());

    for (uint32 i = 0; i < _swapchain_framebuffers.size(); i++) {
        std::array<vk::ImageView, 3> attachments = {
            _color_image_view,
            _depth_image_view,
            _swapchain_image_views[i]
        };

        // Create framebuffer
        vk::FramebufferCreateInfo framebuffer_info{};
        framebuffer_info.setRenderPass(_render_pass);
        framebuffer_info.setAttachments(attachments);
        framebuffer_info.setWidth(_swapchain_extent.width);
        framebuffer_info.setHeight(_swapchain_extent.height);
        framebuffer_info.setLayers(1);

        auto result = _device.createFramebuffer(&framebuffer_info, _allocator, &_swapchain_framebuffers[i]);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create framebuffer.");
    }
}