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

    // Clear color attachment
    if (_color_attachment) {
        if (_color_attachment->internal_data)
            delete _color_attachment->internal_data;
        delete _color_attachment;
    }
    // Clear depth attachment
    if (_depth_attachment) {
        if (_depth_attachment->internal_data)
            delete _depth_attachment->internal_data;
        delete _depth_attachment;
    }
    // Clear render textures
    for (auto& texture : _render_textures) {
        auto data =
            reinterpret_cast<VulkanTextureData*>(texture->internal_data());
        _device->handle().waitIdle();
        if (data->image) delete data->image;
    }
    _render_textures.clear();

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

uint8 VulkanSwapchain::get_current_index() const {
    return _current_image_index;
}

uint8 VulkanSwapchain::get_render_texture_count() const {
    return _render_textures.size();
}
Texture* VulkanSwapchain::get_render_texture(const uint8 index) const {
    if (index >= _render_textures.size())
        Logger::fatal(RENDERER_VULKAN_LOG, "Invalid swapchain index passed.");
    return _render_textures[index];
}

Texture* VulkanSwapchain::get_depth_texture() const {
    return _depth_attachment;
}

Texture* VulkanSwapchain::get_color_texture() const {
    return _color_attachment;
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
            _handle, uint64_max, signal_semaphore
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
    std::array<vk::SwapchainKHR, 1> swapchains { _handle };

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
    }
    // TODO: Windows nvidia drivers throw this (probably). Can be handled
    // without catching using pResults in present_info
    catch (vk::OutOfDateKHRError e) {
        recreate();
        _should_resize = false;
    } catch (vk::SystemError e) {
        Logger::fatal(RENDERER_VULKAN_LOG, e.what());
    }
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
    _format = surface_format.format;
    _extent = extent;

    // Retrieve handles to images created with the swapchain
    auto swapchain_images = _device->handle().getSwapchainImagesKHR(_handle);

    // Check whether render textures already exist
    if (_render_textures.empty()) {
        // Create new swapchain images (render textures)
        for (uint32 i = 0; i < swapchain_images.size(); i++) {
            // Create unique name
            const auto texture_name = String::build(
                "__internal_vulkan_swapchain_render_texture_", i, "__"
            );

            // Create new wrapped texture (Not managed by the texture system)
            Texture* const texture = new (MemoryTag::Texture) Texture(
                texture_name,
                _extent.width,
                _extent.height,
                4,     // Channel count
                false, // Transparent
                true,  // Writable
                true   // Wrapped
            );

            // Fill internal data
            auto internal_data =
                new (MemoryTag::GPUTexture) VulkanTextureData();
            internal_data->image =
                new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
            internal_data->image->create(
                swapchain_images[i], _extent.width, _extent.height
            );
            texture->internal_data = internal_data;

            _render_textures.push_back(texture);
        }
    } else {
        // Just resize
        for (uint32 i = 0; i < swapchain_images.size(); i++) {
            // Resize texture
            _render_textures[i]->resize(extent.width, extent.height);

            // Resize image internal
            auto internal_data = reinterpret_cast<VulkanTextureData*>(
                _render_textures[i]->internal_data()
            );
            internal_data->image->create(
                swapchain_images[i], _extent.width, _extent.height
            );
        }
    }

    // Create swapchain image views
    for (const auto& texture : _render_textures) {
        auto internal_data =
            reinterpret_cast<VulkanTextureData*>(texture->internal_data());

        internal_data->image->create_view(
            1, _format, vk::ImageAspectFlagBits::eColor
        );
    }
}

void VulkanSwapchain::destroy() {
    // Destroy image resources
    if (_color_attachment && _color_attachment->internal_data) {
        auto data = (VulkanTextureData*) _color_attachment->internal_data();
        delete data->image;
    }
    if (_depth_attachment && _depth_attachment->internal_data) {
        auto data = (VulkanTextureData*) _depth_attachment->internal_data();
        delete data->image;
    }

    // Destroy image views
    for (const auto& texture : _render_textures) {
        auto internal_data =
            reinterpret_cast<VulkanTextureData*>(texture->internal_data());
        _device->handle().destroyImageView(
            internal_data->image->view, _allocator
        );
    }

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

    // Recreate synced targets
    recreate_event.invoke(_extent.width, _extent.height);
}

void VulkanSwapchain::create_color_resource() {
    if (_color_attachment == nullptr) {
        // Create new wrapped texture (Not managed by the texture system)
        _color_attachment = new (MemoryTag::Texture) Texture(
            "__default_color_attachment_texture__",
            _extent.width,
            _extent.height,
            4,     // Channel count
            false, // Transparent
            true,  // Writable
            true   // Wrapped
        );
        _color_attachment->internal_data =
            new (MemoryTag::GPUTexture) VulkanTextureData();
    } else {
        // Resize
        _color_attachment->resize(_extent.width, _extent.height);
    }

    // Fill internal data
    const auto data = (VulkanTextureData*) _color_attachment->internal_data();
    data->image = new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    data->image->create(
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
    if (_depth_attachment == nullptr) {
        // Create new wrapped texture (Not managed by the texture system)
        _depth_attachment = new (MemoryTag::Texture) Texture(
            "__default_depth_attachment_texture__",
            _extent.width,
            _extent.height,
            _depth_format_channel_count,
            false, // Transparent
            true,  // Writable
            true   // Wrapped
        );
        _depth_attachment->internal_data =
            new (MemoryTag::GPUTexture) VulkanTextureData();
    } else {
        // Resize
        _depth_attachment->resize(_extent.width, _extent.height);
    }

    // Fill internal data
    const auto data = (VulkanTextureData*) _depth_attachment->internal_data();
    data->image = new (MemoryTag::GPUTexture) VulkanImage(_device, _allocator);
    data->image->create(
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
    // Potential depth formats with their channel count
    Vector<std::pair<vk::Format, uint8>> candidates {
        { vk::Format::eD32Sfloat, 4 },
        { vk::Format::eD32SfloatS8Uint, 4 },
        { vk::Format::eD24UnormS8Uint, 3 }
    };
    vk::FormatFeatureFlags features =
        vk::FormatFeatureFlagBits::eDepthStencilAttachment;

    // Find a format among the candidates that satisfies the tiling and
    // feature requirements
    for (auto format : candidates) {
        auto properties = _device->info().get_format_properties(format.first);
        if ((features & properties.linearTilingFeatures) == features) {
            _depth_format               = format.first;
            _depth_format_channel_count = format.second;
            return;
        } else if ((features & properties.optimalTilingFeatures) == features) {
            _depth_format               = format.first;
            _depth_format_channel_count = format.second;
            return;
        }
    }
    Logger::fatal(RENDERER_VULKAN_LOG, "Failed to find supported format.");
}

} // namespace ENGINE_NAMESPACE