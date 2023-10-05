#include "resources/texture.hpp"

namespace ENGINE_NAMESPACE {

// Statics values
const uint32 Texture::max_name_length;

Texture::Texture(
    const String name,
    const int32  width,
    const int32  height,
    const int32  channel_count,
    const bool   has_transparency
)
    : _name(name), _width(width), _height(height),
      _channel_count(channel_count), _has_transparency(has_transparency) {
    _total_size = width * height * channel_count;
}

} // namespace ENGINE_NAMESPACE