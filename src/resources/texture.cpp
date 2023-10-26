#include "resources/texture.hpp"
#include <cmath>

namespace ENGINE_NAMESPACE {

// Statics values
const uint32 Texture::max_name_length;

Texture::Texture(
    const String name,
    const int32  width,
    const int32  height,
    const int32  channel_count,
    const bool   has_transparency,
    const bool   is_writable
)
    : _name(name), _width(width), _height(height),
      _channel_count(channel_count), _has_transparency(has_transparency),
      _is_writable(is_writable) {
    _total_size = width * height * channel_count;
    _mip_levels = (uint32) std::floor(std::log2(std::max(_width, _height))) + 1;
}

} // namespace ENGINE_NAMESPACE