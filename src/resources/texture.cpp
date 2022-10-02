#include "resources/texture.hpp"

#define TEXTURES_FOLDER "./assets/textures/"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <system_error>

Texture::Texture(
    const int32 width,
    const int32 height,
    const int32 channel_count,
    const String name,
    const bool has_transparency
) : _width(width), _height(height), _channel_count(channel_count),
_name(name), _has_transparency(has_transparency) {
    _total_size = width * height * channel_count;
}
Texture::Texture(
    const String& name,
    const String& extension,
    byte*& out_data
) {
    // TODO: TEMP, will be replaced by the resource system
    // Load image
    int32 image_width, image_height, image_channels;
    String file_name = name + "." + extension; file_name.to_lower();
    String file_path = TEXTURES_FOLDER + file_name;
    stbi_uc* image_pixels =
        stbi_load(file_path.c_str(), &image_width, &image_height, &image_channels, STBI_rgb_alpha);

    if (!image_pixels) {
        throw std::runtime_error("Failed to load texture image.");
        stbi_image_free(image_pixels);
    }

    _width = image_width;
    _height = image_height;
    _channel_count = 4;
    _total_size = _width * _height * _channel_count;

    this->_name = name;
    out_data = (byte*) image_pixels;

    // Check for transparency
    _has_transparency = false;
    if (image_channels > 3) {
        for (uint64 i = 3; i < _total_size; i += _channel_count) {
            if (out_data[i] < (byte) 255) {
                _has_transparency = true;
                break;
            }
        }
    }
}