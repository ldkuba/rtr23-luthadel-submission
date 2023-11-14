#include "resources/texture.hpp"
#include <cmath>

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
Texture::Config::Config(
    const String name,
    const uint32 width,
    const uint32 height,
    const uint32 channel_count,
    const bool   mip_mapping,
    const bool   has_transparency,
    const bool   is_writable,
    const bool   is_wrapped
)
    : name(name), width(width), height(height), channel_count(channel_count),
      has_transparency(has_transparency), is_writable(is_writable),
      is_wrapped(is_wrapped),
      mip_level_count(
          (mip_mapping)
              ? (uint8) std::floor(std::log2(std::max(width, height))) + 1
              : 1
      ) {}

Texture::Texture(const Config& config)
    : _name(config.name), _width(config.width), _height(config.height),
      _channel_count(config.channel_count), _mip_levels(config.mip_level_count),
      _flags(0) {
    _total_size = width * height * channel_count;
    if (config.has_transparency) _flags |= HasTransparency;
    if (config.is_writable) _flags |= IsWritable;
    if (config.is_wrapped) _flags |= IsWrapped;
}

// ////////////////////// //
// TEXTURE PUBLIC METHODS //
// ////////////////////// //

Outcome Texture::write(const Vector<byte>& data, const uint32 offset) {
    return write(data.data(), data.size(), offset);
}

Outcome Texture::write(
    const byte* const data, const uint32 size, const uint32 offset
) {
    return Outcome::Successful;
}

Outcome Texture::resize(const uint32 width, const uint32 height) {
    if (!is_writable()) return Outcome::Failed;
    _width  = width;
    _height = height;
    return Outcome::Successful;
}

} // namespace ENGINE_NAMESPACE