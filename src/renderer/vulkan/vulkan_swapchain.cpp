#include "renderer/vulkan/vulkan_backend.hpp"

#include "logger.hpp"

void VulkanBackend::create_swapchain() {
    // Get swapchain details
    SwapchainSupportDetails swapchain_support = _device.info.get_swapchain_support_details(_vulkan_surface);

    vk::Extent2D extent = swapchain_support.get_extent(_surface->get_width_in_pixels(), _surface->get_height_in_pixels());
    vk::SurfaceFormatKHR surface_format = swapchain_support.get_surface_format();
    vk::PresentModeKHR presentation_mode = swapchain_support.get_presentation_mode();

    uint32 min_image_count = swapchain_support.capabilities.minImageCount + 1;
    uint32 max_image_count = swapchain_support.capabilities.maxImageCount;
    if (max_image_count != 0 && min_image_count > max_image_count)
        min_image_count = max_image_count;

    // Create swapchain
    vk::SwapchainCreateInfoKHR create_info{};
    create_info.setSurface(_vulkan_surface);                                //
    create_info.setMinImageCount(min_image_count);                          //
    create_info.setImageExtent(extent);                                     //
    create_info.setImageFormat(surface_format.format);                      //
    create_info.setImageColorSpace(surface_format.colorSpace);              //
    create_info.setPresentMode(presentation_mode);                          //
    create_info.setImageArrayLayers(1);                                     //
    create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);    // Render to color buffer; TODO: postprocessing
    create_info.setPreTransform(swapchain_support.capabilities.currentTransform); //
    create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);  // Composite with the operating system
    create_info.setClipped(true);                                           // Clip object beyond screen
    create_info.setOldSwapchain(VK_NULL_HANDLE);                            // 

    // Setup queue family indices
    std::array<uint32, 2> queue_family_indices{
        _device.queue_family_indices.graphics_family.value(),
        _device.queue_family_indices.present_family.value()
    };
    if (_device.queue_family_indices.graphics_family.value() != _device.queue_family_indices.present_family.value()) {
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
        create_info.setQueueFamilyIndices(queue_family_indices);
    } else {
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        create_info.setQueueFamilyIndexCount(0);
        create_info.setPQueueFamilyIndices(nullptr);
    }

    try {
        _swapchain = _device.handle.createSwapchainKHR(create_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }

    // Create swapchain images
    auto swapchain_images = _device.handle.getSwapchainImagesKHR(_swapchain);
    _swapchain_format = surface_format.format;
    _swapchain_extent = extent;

    // Create swapchain image views
    _swapchain_image_views.resize(swapchain_images.size());
    for (uint32 i = 0; i < swapchain_images.size(); i++) {
        _swapchain_image_views[i] = VulkanImage::get_view_from_image(
            _swapchain_format,
            vk::ImageAspectFlagBits::eColor,
            swapchain_images[i],
            _device.handle,
            _allocator
        );
    }
}

void VulkanBackend::recreate_swapchain() {
    _device.handle.waitIdle();

    cleanup_swapchain();

    create_swapchain();
    create_color_resource();
    create_depth_resources();
    create_framebuffers();
}

void VulkanBackend::cleanup_swapchain() {
    // TODO: TEMP MSAA CODE
    _color_image.~VulkanImage();


    // TODO: TEMP DEPTH BUFFER CODE
    _depth_image.~VulkanImage();


    // TODO: TEMP FRAMEBUFFER CODE
    for (auto framebuffer : _swapchain_framebuffers)
        _device.handle.destroyFramebuffer(framebuffer, _allocator);


    for (auto image_view : _swapchain_image_views)
        _device.handle.destroyImageView(image_view, _allocator);

    _device.handle.destroySwapchainKHR(_swapchain, _allocator);
}

// FRAMEBUFFER
void VulkanBackend::create_framebuffers() {
    _swapchain_framebuffers.resize(_swapchain_image_views.size());

    for (uint32 i = 0; i < _swapchain_framebuffers.size(); i++) {
        std::array<vk::ImageView, 3> attachments = {
            _color_image.view,
            _depth_image.view,
            _swapchain_image_views[i]
        };

        // Create framebuffer
        vk::FramebufferCreateInfo framebuffer_info{};
        framebuffer_info.setRenderPass(_render_pass);
        framebuffer_info.setAttachments(attachments);
        framebuffer_info.setWidth(_swapchain_extent.width);
        framebuffer_info.setHeight(_swapchain_extent.height);
        framebuffer_info.setLayers(1);

        auto result = _device.handle.createFramebuffer(&framebuffer_info, _allocator, &_swapchain_framebuffers[i]);
        if (result != vk::Result::eSuccess)
            throw std::runtime_error("Failed to create framebuffer.");
    }
}

void VulkanBackend::present_swapchain() {
    // Wait for previous frame to finish drawing
    auto result = _device.handle.waitForFences(1, &_fences_in_flight[current_frame], true, UINT64_MAX);
    if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to draw frame.");

    // Obtain a swapchain image (next in queue for drawing)
    auto obtained = _device.handle.acquireNextImageKHR(_swapchain, UINT64_MAX, _semaphores_image_available[current_frame]);
    if (obtained.result == vk::Result::eErrorOutOfDateKHR) {
        recreate_swapchain();
        return;
    } else if (obtained.result != vk::Result::eSuccess && obtained.result != vk::Result::eSuboptimalKHR) {
        Logger::fatal("Failed to obtain a swapchain image.");
    }
    auto image_index = obtained.value;

    // Reset fence
    result = _device.handle.resetFences(1, &_fences_in_flight[current_frame]);
    if (result != vk::Result::eSuccess) throw std::runtime_error("Failed to draw frame.");

    // Record commands
    _command_buffers[current_frame].reset();
    record_command_buffer(_command_buffers[current_frame], image_index);

    // Update uniform buffer data
    update_uniform_buffer(current_frame);

    // Submit command buffer
    vk::Semaphore wait_semaphores[] = { _semaphores_image_available[current_frame] };
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signal_semaphores[] = { _semaphores_render_finished[current_frame] };

    vk::SubmitInfo submit_info{};
    submit_info.setWaitSemaphoreCount(1);
    submit_info.setPWaitSemaphores(wait_semaphores);
    submit_info.setPWaitDstStageMask(wait_stages);
    submit_info.setCommandBufferCount(1);
    submit_info.setPCommandBuffers(&_command_buffers[current_frame]);
    submit_info.setSignalSemaphoreCount(1);
    submit_info.setPSignalSemaphores(signal_semaphores);

    result = _device.graphics_queue.submit(1, &submit_info, _fences_in_flight[current_frame]);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to submit draw command buffer.");

    // Present results
    std::vector<vk::SwapchainKHR> swapchains = { _swapchain };

    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphoreCount(1);
    present_info.setPWaitSemaphores(signal_semaphores); // Semaphores to wait on before presenting
    present_info.setSwapchains(swapchains);             // List of presenting swapchains
    present_info.setPImageIndices(&image_index);        // Index of presenting image for each swapchain
    present_info.setPResults(nullptr);                  // Check if presentation is successful for each swapchain (not needed)

    result = _device.presentation_queue.presentKHR(present_info);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _surface->resized) {
        recreate_swapchain();
        _surface->resized = false;
    } else if (result != vk::Result::eSuccess) {
        Logger::fatal("Failed to present rendered image.");
    }

    // Advance current frame
    current_frame = (current_frame + 1) % VulkanSettings::max_frames_in_flight;
}