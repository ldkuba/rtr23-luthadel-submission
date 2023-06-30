#include "resources/loaders/image_loader.hpp"

#include "systems/resource_system.hpp"
#include "resources/image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Supported extensions
const std::vector<String> ImageLoader::_supported_extensions = {
    ".png", ".jpg", ".tga", ".bmp"
};

// Constructor & Destructor
ImageLoader::ImageLoader() {
    _type      = ResourceType::Image;
    _type_path = "textures";
}
ImageLoader::~ImageLoader() {}

// /////////////////////////// //
// IMAGE LOADER PUBLIC METHODS //
// /////////////////////////// //

Result<Resource*, RuntimeError> ImageLoader::load(const String name) {
    // Compute full path
    String file_path =
        ResourceSystem::base_path + "/" + _type_path + "/" + name;

    // Required channel cont
    const uint32 req_channel_count = 4;

    // Image info
    int32 image_width, image_height, image_channels;

    // Load image
    stbi_uc* image_pixels = nullptr;
    if (name.split('.').size() < 2) {
        // If name is not provided with an extension, try few default default
        // extensions
        for (const auto& extension : _supported_extensions) {
            image_pixels = stbi_load(
                (file_path + extension).c_str(),
                &image_width,
                &image_height,
                &image_channels,
                req_channel_count
            );
            if (image_pixels) break;
        }
    } else {
        image_pixels = stbi_load(
            file_path.c_str(),
            &image_width,
            &image_height,
            &image_channels,
            req_channel_count
        );
    }

    // Check if successful
    if (!image_pixels) {
        stbi_image_free(image_pixels);
        String error_message = "Failed to load texture image.";
        Logger::error(RESOURCE_LOG, error_message);
        return Failure(error_message);
    }

    // Check for transparency
    auto   has_transparency = false;
    uint64 total_size       = image_width * image_height * req_channel_count;
    if (image_channels > 3) {
        for (uint64 i = 3; i < total_size; i += req_channel_count) {
            if (image_pixels[i] < (stbi_uc) 255) {
                has_transparency = true;
                break;
            }
        }
    }

    // Return image data
    Image* image = new (MemoryTag::Resource) Image(
        name, image_width, image_height, req_channel_count, (byte*) image_pixels
    );
    image->full_path   = file_path;
    image->loader_type = ResourceType::Image;
    return image;
}
void ImageLoader::unload(Resource* resource) {
    can_unload(ResourceType::Image, resource);

    Image* res = (Image*) resource;
    delete res;
}
