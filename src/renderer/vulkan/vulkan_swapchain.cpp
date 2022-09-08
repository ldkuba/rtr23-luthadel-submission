#include "renderer/vulkan/vulkan_settings.hpp"
#include "renderer/vulkan/vulkan_swapchain.hpp"

// Constructor & Destructor
VulkanSwapchain::VulkanSwapchain(
    vk::SurfaceKHR vulkan_surface,
    Platform::Surface* surface,
    VulkanDevice* device,
    vk::AllocationCallbacks* allocator
) : _vulkan_surface(vulkan_surface), _surface(surface), _device(device), _allocator(allocator) {
    // Set maximum MSAA samples
    auto count = _device->info.framebuffer_color_sample_counts &
        _device->info.framebuffer_depth_sample_counts;

    msaa_samples = vk::SampleCountFlagBits::e1;
    if (count & vk::SampleCountFlagBits::e64) msaa_samples = vk::SampleCountFlagBits::e64;
    else if (count & vk::SampleCountFlagBits::e32) msaa_samples = vk::SampleCountFlagBits::e32;
    else if (count & vk::SampleCountFlagBits::e16) msaa_samples = vk::SampleCountFlagBits::e16;
    else if (count & vk::SampleCountFlagBits::e8) msaa_samples = vk::SampleCountFlagBits::e8;
    else if (count & vk::SampleCountFlagBits::e4) msaa_samples = vk::SampleCountFlagBits::e4;
    else if (count & vk::SampleCountFlagBits::e2) msaa_samples = vk::SampleCountFlagBits::e2;

    if (msaa_samples > VulkanSettings::max_msaa_samples)
        msaa_samples = VulkanSettings::max_msaa_samples;

    // Create swapchain proper
    create();

    // Create vulkan images
    _depth_image = new VulkanImage(_device, _allocator);
    create_depth_resources();
    _color_image = new VulkanImage(_device, _allocator);
    create_color_resource();
}

VulkanSwapchain::~VulkanSwapchain() {
    destroy();
}

// /////////////// //
// Private methods //
// /////////////// //

void VulkanSwapchain::create() {
    // Get swapchain details
    SwapchainSupportDetails swapchain_support = _device->info.get_swapchain_support_details(_vulkan_surface);

    vk::Extent2D extent = swapchain_support.get_extent(_surface->get_width_in_pixels(), _surface->get_height_in_pixels());
    vk::SurfaceFormatKHR surface_format = swapchain_support.get_surface_format();
    vk::PresentModeKHR presentation_mode = swapchain_support.get_presentation_mode();

    // Decide on how many images the swapchain should have to function
    // (Here 1 more than the minimum amount required)
    uint32 min_image_count = swapchain_support.capabilities.minImageCount + 1;
    // Make sure the maximum number of images is not exceeded (0 here is a special number, meaning no maximum)
    uint32 max_image_count = swapchain_support.capabilities.maxImageCount;
    if (max_image_count != 0 && min_image_count > max_image_count)
        min_image_count = max_image_count;

    // Create swapchain
    vk::SwapchainCreateInfoKHR create_info{};
    create_info.setSurface(_vulkan_surface);                                // Surface the swapchain will be tied to
    create_info.setMinImageCount(min_image_count);                          // Number of swapchain images used
    create_info.setImageExtent(extent);                                     // Swapchain image dimensions
    create_info.setImageFormat(surface_format.format);                      // Swapchain image format
    create_info.setImageColorSpace(surface_format.colorSpace);              // Swapchain image color space
    // Algorithm to be used to determine the order of image rendering and presentation
    create_info.setPresentMode(presentation_mode);
    create_info.setImageArrayLayers(1);                                     // Amount of layers each image consists of
    create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);    // Render to color buffer
    // Transform to be applied to image in swapchain (currentTransform for selecting none)
    create_info.setPreTransform(swapchain_support.capabilities.currentTransform);
    create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);  // Composite with the operating system
    create_info.setClipped(true);                                           // Clip object beyond screen
    create_info.setOldSwapchain(VK_NULL_HANDLE);                            // TODO: More optimal swapchain recreation

    // Specify how to handle swapchain images which are used across multiple queue families
    // The two interesting queues in this case are graphics and present queues
    std::array<uint32, 2> queue_family_indices{
        _device->queue_family_indices.graphics_family.value(),
        _device->queue_family_indices.present_family.value()
    };
    if (_device->queue_family_indices.graphics_family.value() != _device->queue_family_indices.present_family.value()) {
        // Multiple queue families can access an image
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent);
        create_info.setQueueFamilyIndices(queue_family_indices);
    } else {
        // Only one queue family can access an image without explicit transfer
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
        create_info.setQueueFamilyIndexCount(0);
        create_info.setPQueueFamilyIndices(nullptr);
    }

    try {
        _handle = _device->handle.createSwapchainKHR(create_info, _allocator);
    } catch (const vk::SystemError& e) { Logger::fatal(e.what()); }


    // Remember swapchain format and extent
    _format = surface_format.format;
    this->extent = extent;

    // Retrieve handles to images created with the swapchain
    auto swapchain_images = _device->handle.getSwapchainImagesKHR(_handle);
    // Create swapchain image views
    _image_views.resize(swapchain_images.size());
    for (uint32 i = 0; i < swapchain_images.size(); i++) {
        _image_views[i] = VulkanImage::get_view_from_image(
            _format,
            vk::ImageAspectFlagBits::eColor,
            swapchain_images[i],
            _device->handle,
            _allocator
        );
    }
}

void VulkanSwapchain::destroy() {
    // Destroy image resources
    _color_image->~VulkanImage();
    _depth_image->~VulkanImage();

    // Destroy framebuffers
    for (auto framebuffer : framebuffers)
        _device->handle.destroyFramebuffer(framebuffer, _allocator);

    // Destroy image views
    for (auto image_view : _image_views)
        _device->handle.destroyImageView(image_view, _allocator);

    // Destroy handle
    _device->handle.destroySwapchainKHR(_handle, _allocator);
}

void VulkanSwapchain::recreate() {
    // Finish all rendering
    _device->handle.waitIdle();

    // Destroy previous swapchain resources
    destroy();

    // Create new swapchain resources
    create();
    create_color_resource();
    create_depth_resources();
    create_framebuffers();
}

void VulkanSwapchain::create_color_resource() {
    vk::Format color_format = _format;

    _color_image->create(
        extent.width, extent.height, 1,
        msaa_samples,
        color_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );
}
void VulkanSwapchain::create_depth_resources() {
    auto depth_format = find_depth_format();

    _depth_image->create(
        extent.width, extent.height, 1,
        msaa_samples,
        depth_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eDepth
    );
}

vk::Format VulkanSwapchain::find_depth_format() {
    std::vector<vk::Format> candidates = {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint
    };
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
    vk::FormatFeatureFlags features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;

    // Find a format among the candidates that satisfies the tiling and feature requirements
    for (auto format : candidates) {
        auto properties = _device->info.get_format_properties(format);
        vk::FormatFeatureFlags supported_features;
        if (tiling == vk::ImageTiling::eLinear && (features & properties.linearTilingFeatures) == features)
            return format;
        else if (tiling == vk::ImageTiling::eOptimal && (features & properties.optimalTilingFeatures) == features)
            return format;
    }
    Logger::fatal("Failed to find supported format.");
    return vk::Format();
}

void VulkanSwapchain::create_framebuffers() {
    framebuffers.resize(_image_views.size());

    // Create a framebuffer for each swapchain image view
    for (uint32 i = 0; i < framebuffers.size(); i++) {
        std::array<vk::ImageView, 3> attachments = {
            _color_image->view,
            _depth_image->view,
            _image_views[i]
        };

        // Create framebuffer
        vk::FramebufferCreateInfo framebuffer_info{};
        framebuffer_info.setRenderPass(*_render_pass); // Render pass with which framebuffer need to be compatible
        // List of objects bound to the corresponding attachment descriptions render pass
        framebuffer_info.setAttachments(attachments);
        framebuffer_info.setWidth(extent.width);       // Framebuffer width
        framebuffer_info.setHeight(extent.height);     // Framebuffer height
        framebuffer_info.setLayers(1);                 // Number of layers in image array

        try {
            framebuffers[i] = _device->handle.createFramebuffer(framebuffer_info, _allocator);
        } catch (vk::SystemError e) { Logger::fatal(e.what()); }
    }
}

// ////////////// //
// Public methods //
// ////////////// //

void VulkanSwapchain::initialize_framebuffers(vk::RenderPass* render_pass) {
    _render_pass = render_pass; // Set render pass requirement
    create_framebuffers();      // Create framebuffer
}

vk::AttachmentDescription VulkanSwapchain::get_depth_attachment() {
    vk::AttachmentDescription depth_attachment{};
    depth_attachment.setFormat(find_depth_format());
    depth_attachment.setSamples(msaa_samples);
    depth_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    depth_attachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
    depth_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    depth_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    depth_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
    depth_attachment.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    return depth_attachment;
}

vk::AttachmentDescription VulkanSwapchain::get_color_attachment() {
    vk::AttachmentDescription color_attachment{};
    color_attachment.setFormat(_format);
    color_attachment.setSamples(msaa_samples);
    color_attachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    color_attachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment.setInitialLayout(vk::ImageLayout::eUndefined);
    color_attachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    return color_attachment;
}

vk::AttachmentDescription VulkanSwapchain::get_color_attachment_resolve() {
    vk::AttachmentDescription color_attachment_resolve{};
    color_attachment_resolve.setFormat(_format);
    color_attachment_resolve.setSamples(vk::SampleCountFlagBits::e1);
    color_attachment_resolve.setLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment_resolve.setStoreOp(vk::AttachmentStoreOp::eStore);
    color_attachment_resolve.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    color_attachment_resolve.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    color_attachment_resolve.setInitialLayout(vk::ImageLayout::eUndefined);
    color_attachment_resolve.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    return color_attachment_resolve;
}

uint32 VulkanSwapchain::acquire_next_image_index(vk::Semaphore signal_semaphore) {
    // Obtain a swapchain image (next in queue for drawing)
    auto obtained = _device->handle.acquireNextImageKHR(_handle, UINT64_MAX, signal_semaphore);
    if (obtained.result == vk::Result::eErrorOutOfDateKHR) {
        // Surface changed, and so swapchain needs to be recreated
        recreate();
        return -1;
    } else if (obtained.result != vk::Result::eSuccess && obtained.result != vk::Result::eSuboptimalKHR) {
        Logger::fatal("Failed to obtain a swapchain image.");
    }
    return obtained.value;
}

void VulkanSwapchain::present(uint32 image_index, std::vector<vk::Semaphore> wait_for_semaphores) {
    std::vector<vk::SwapchainKHR> swapchains = { _handle };

    // Present results
    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphores(wait_for_semaphores); // Semaphores to wait on before presenting
    present_info.setSwapchains(swapchains);              // List of presenting swapchains
    present_info.setPImageIndices(&image_index);         // Index of presenting image for each swapchain
    // Check if presentation is successful for each swapchain (not needed, as there is only 1)
    present_info.setPResults(nullptr);

    auto result = _device->presentation_queue.presentKHR(present_info);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || _surface->resized) {
        // Surface changed, and so swapchain needs to be recreated
        recreate();
        _surface->resized = false;
    } else if (result != vk::Result::eSuccess) {
        Logger::fatal("Failed to present rendered image.");
    }
}
