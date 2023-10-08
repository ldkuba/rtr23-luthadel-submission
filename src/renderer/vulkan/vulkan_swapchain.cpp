#include "renderer/vulkan/vulkan_swapchain.hpp"

#include "renderer/vulkan/vulkan_settings.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
VulkanSwapchain::VulkanSwapchain(
    const VulkanDevice* const            device,
    const vk::AllocationCallbacks* const allocator,
    const uint32                         width,
    const uint32                         height,
    const vk::SurfaceKHR                 vulkan_surface
)
    : _width(width), _height(height), _vulkan_surface(vulkan_surface),
      _device(device), _allocator(allocator) {

    // Set maximum MSAA samples
    auto count = _device->info().framebuffer_color_sample_counts &
                 _device->info().framebuffer_depth_sample_counts;

    _msaa_samples = vk::SampleCountFlagBits::e1;
    if (count & vk::SampleCountFlagBits::e64)
        _msaa_samples = vk::SampleCountFlagBits::e64;
    else if (count & vk::SampleCountFlagBits::e32)
        _msaa_samples = vk::SampleCountFlagBits::e32;
    else if (count & vk::SampleCountFlagBits::e16)
        _msaa_samples = vk::SampleCountFlagBits::e16;
    else if (count & vk::SampleCountFlagBits::e8)
        _msaa_samples = vk::SampleCountFlagBits::e8;
    else if (count & vk::SampleCountFlagBits::e4)
        _msaa_samples = vk::SampleCountFlagBits::e4;
    else if (count & vk::SampleCountFlagBits::e2)
        _msaa_samples = vk::SampleCountFlagBits::e2;

    if (_msaa_samples > VulkanSettings::max_msaa_samples)
        _msaa_samples = VulkanSettings::max_msaa_samples;

    // Create swapchain proper
    Logger::trace(RENDERER_VULKAN_LOG, "Creating swapchain.");
    create();
    Logger::trace(RENDERER_VULKAN_LOG, "Swapchain created.");

    // Create vulkan images
    find_depth_format();
    create_depth_resources();
    create_color_resource();
}

VulkanSwapchain::~VulkanSwapchain() {
    destroy();
    for (auto& framebuffer_set : _framebuffer_sets) {
        for (auto& framebuffer : framebuffer_set.framebuffers)
            del(framebuffer);
        framebuffer_set.framebuffers.clear();
    }
    _framebuffer_sets.clear();
    Logger::trace(RENDERER_VULKAN_LOG, "Swapchain destroyed.");
}

// /////////////////////////////// //
// VULKAN SWAPCHAIN PUBLIC METHODS //
// /////////////////////////////// //

void VulkanSwapchain::change_extent(const uint32 width, const uint32 height) {
    _width         = width;
    _height        = height;
    _should_resize = true;
}

uint32 VulkanSwapchain::create_framebuffers(
    const VulkanRenderPass* const render_pass,
    bool                          multisampling,
    bool                          depth_testing
) {
    Logger::trace(RENDERER_VULKAN_LOG, "Initializing framebuffers.");

    Vector<VulkanFramebuffer*> framebuffers { _image_views.size() };
    for (uint32 i = 0; i < framebuffers.size(); i++) {
        Vector<vk::ImageView> attachments {};
        if (multisampling) attachments.push_back(_color_image->view());
        if (depth_testing) attachments.push_back(_depth_image->view());
        attachments.push_back(_image_views[i]);

        framebuffers[i] = new (MemoryTag::GPUBuffer) VulkanFramebuffer(
            &_device->handle(),
            _allocator,
            render_pass,
            _extent.width,
            _extent.height,
            attachments
        );
    }
    _framebuffer_sets.push_back({ framebuffers, multisampling, depth_testing });

    Logger::trace(RENDERER_VULKAN_LOG, "Framebuffers initialized.");
    return _framebuffer_sets.size() - 1;
}

vk::Format VulkanSwapchain::get_color_attachment_format() const {
    return _format;
}
vk::Format VulkanSwapchain::get_depth_attachment_format() const {
    return _depth_format;
}

void VulkanSwapchain::compute_next_image_index(
    const vk::Semaphore& signal_semaphore
) {
    // Obtain a swapchain image (next in queue for drawing)
    try {
        auto obtained = _device->handle().acquireNextImageKHR(
            _handle, UINT64_MAX, signal_semaphore
        );
        if (obtained.result != vk::Result::eSuccess &&
            obtained.result != vk::Result::eSuboptimalKHR)
            Logger::fatal(
                RENDERER_VULKAN_LOG,
                "Swapchain image index acquisition timed-out."
            );
        _current_image_index = obtained.value;
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }
}

void VulkanSwapchain::present(
    const vk::ArrayProxyNoTemporaries<vk::Semaphore>& wait_for_semaphores
) {
    std::array<vk::SwapchainKHR, 1> swapchains = { _handle };

    // Present results
    vk::PresentInfoKHR present_info {};
    // Semaphores to wait on before presenting
    present_info.setWaitSemaphores(wait_for_semaphores);
    // List of presenting swapchains
    present_info.setSwapchains(swapchains);
    // Index of presenting image for each swapchain.
    present_info.setPImageIndices(&_current_image_index);
    // Check if presentation is successful for each swapchain (not needed, as
    // there is only 1 swapchain)
    present_info.setPResults(nullptr);

    try {
        auto result = _device->presentation_queue.presentKHR(present_info);
        if (result == vk::Result::eSuboptimalKHR || _should_resize) {
            // Surface changed, and so swapchain needs to be recreated
            recreate();
            _should_resize = false;
        }
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }
}

VulkanFramebuffer* VulkanSwapchain::get_currently_used_framebuffer(
    const uint32 index
) {
    if (index > _framebuffer_sets.size())
        Logger::fatal(RENDERER_VULKAN_LOG, "Framebuffer index out of range.");
    return _framebuffer_sets[index].framebuffers[_current_image_index];
}

// //////////////////////////////// //
// VULKAN SWAPCHAIN PRIVATE METHODS //
// //////////////////////////////// //

void VulkanSwapchain::create() {
    // === Get swapchain details ===
    SwapchainSupportDetails swapchain_support =
        _device->info().get_swapchain_support_details(_vulkan_surface);

    vk::Extent2D         extent = swapchain_support.get_extent(_width, _height);
    vk::SurfaceFormatKHR surface_format =
        swapchain_support.get_surface_format();
    vk::PresentModeKHR presentation_mode =
        swapchain_support.get_presentation_mode();

    // Decide on how many images the swapchain should have to function
    // (Here 1 more than the minimum amount required)
    uint32 min_image_count = swapchain_support.capabilities.minImageCount + 1;
    // Make sure the maximum number of images is not exceeded (0 here is a
    // special number, meaning no maximum)
    uint32 max_image_count = swapchain_support.capabilities.maxImageCount;
    if (max_image_count != 0 && min_image_count > max_image_count)
        min_image_count = max_image_count;

    // === Create swapchain ===
    vk::SwapchainCreateInfoKHR create_info {};
    // Surface the swapchain will be tied to
    create_info.setSurface(_vulkan_surface);
    // Number of swapchain images used
    create_info.setMinImageCount(min_image_count);
    // Swapchain image dimensions
    create_info.setImageExtent(extent);
    // Swapchain image format
    create_info.setImageFormat(surface_format.format);
    // Swapchain image color space
    create_info.setImageColorSpace(surface_format.colorSpace);
    // Algorithm to be used to determine the order of image rendering and
    // presentation
    create_info.setPresentMode(presentation_mode);
    // Amount of layers each image consists of
    create_info.setImageArrayLayers(1);
    // Render to color buffer
    create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    // Transform to be applied to image in swapchain (currentTransform for
    // selecting none)
    create_info.setPreTransform(swapchain_support.capabilities.currentTransform
    );
    // Composite with the operating system
    create_info.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
    // Clip object beyond screen
    create_info.setClipped(true);
    // TODO: More optimal swapchain recreation
    create_info.setOldSwapchain(VK_NULL_HANDLE);

    // Specify how to handle swapchain images which are used across multiple
    // queue families The two interesting queues in this case are graphics and
    // present queues
    std::array<uint32, 2> queue_family_indices {
        _device->queue_family_indices.graphics_family.value(),
        _device->queue_family_indices.present_family.value()
    };
    if (_device->queue_family_indices.graphics_family.value() !=
        _device->queue_family_indices.present_family.value()) {
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
        _handle = _device->handle().createSwapchainKHR(create_info, _allocator);
    } catch (const vk::SystemError& e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }

    // === Remember swapchain format and extent ===
    _format       = surface_format.format;
    this->_extent = extent;

    // Retrieve handles to images created with the swapchain
    auto swapchain_images = _device->handle().getSwapchainImagesKHR(_handle);
    // Create swapchain image views
    _image_views.resize(swapchain_images.size());
    for (uint32 i = 0; i < swapchain_images.size(); i++) {
        _image_views[i] = VulkanImage::get_view_from_image(
            _format,
            vk::ImageAspectFlagBits::eColor,
            swapchain_images[i],
            _device->handle(),
            _allocator
        );
    }
}

void VulkanSwapchain::destroy() {
    // Destroy image resources
    del(_color_image);
    del(_depth_image);

    // Destroy image views
    for (auto image_view : _image_views)
        _device->handle().destroyImageView(image_view, _allocator);

    // Destroy handle
    _device->handle().destroySwapchainKHR(_handle, _allocator);
}

void VulkanSwapchain::recreate() {
    // Finish all rendering
    _device->handle().waitIdle();

    // Destroy previous swapchain resources
    destroy();

    // Create new swapchain resources
    create();
    create_color_resource();
    create_depth_resources();

    // Framebuffer recreation
    for (auto& framebuffer_set : _framebuffer_sets) {
        for (uint32 i = 0; i < framebuffer_set.framebuffers.size(); i++) {
            Vector<vk::ImageView> attachments {};
            if (framebuffer_set.multisampling)
                attachments.push_back(_color_image->view());
            if (framebuffer_set.depth_testing)
                attachments.push_back(_depth_image->view());
            attachments.push_back(_image_views[i]);
            framebuffer_set.framebuffers[i]->recreate(
                _extent.width, _extent.height, attachments
            );
        }
    }
}

void VulkanSwapchain::create_color_resource() {
    _color_image = new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    _color_image->create(
        _extent.width,
        _extent.height,
        1,
        _msaa_samples,
        _format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eColor
    );
}
void VulkanSwapchain::create_depth_resources() {
    _depth_image = new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    _depth_image->create(
        _extent.width,
        _extent.height,
        1,
        _msaa_samples,
        _depth_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::ImageAspectFlagBits::eDepth
    );
}

void VulkanSwapchain::find_depth_format() {
    Vector<vk::Format>     candidates = { vk::Format::eD32Sfloat,
                                          vk::Format::eD32SfloatS8Uint,
                                          vk::Format::eD24UnormS8Uint };
    vk::ImageTiling        tiling     = vk::ImageTiling::eOptimal;
    vk::FormatFeatureFlags features =
        vk::FormatFeatureFlagBits::eDepthStencilAttachment;

    // Find a format among the candidates that satisfies the tiling and feature
    // requirements
    for (auto format : candidates) {
        auto properties = _device->info().get_format_properties(format);
        vk::FormatFeatureFlags supported_features;
        if (tiling == vk::ImageTiling::eLinear &&
            (features & properties.linearTilingFeatures) == features) {
            _depth_format = format;
            return;
        } else if (tiling == vk::ImageTiling::eOptimal && (features & properties.optimalTilingFeatures) == features) {
            _depth_format = format;
            return;
        }
    }
    Logger::fatal(RENDERER_VULKAN_LOG, "Failed to find supported format.");
}

} // namespace ENGINE_NAMESPACE