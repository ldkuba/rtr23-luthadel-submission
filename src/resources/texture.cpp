#include "resources/texture.hpp"
#include <cmath>

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
Texture::Texture(
    const String name,
    const int32  width,
    const int32  height,
    const int32  channel_count,
    const bool   has_transparency,
    const bool   is_writable,
    const bool   is_wrapped
)
    : _name(name), _width(width), _height(height),
      _channel_count(channel_count), _flags(0) {
    _total_size = width * height * channel_count;
    _mip_levels = (uint32) std::floor(std::log2(std::max(_width, _height))) + 1;
    if (has_transparency) _flags |= HasTransparency;
    if (is_writable) _flags |= IsWritable;
    if (is_wrapped) _flags |= IsWrapped;
}

// ////////////////////// //
// TEXTURE PUBLIC METHODS //
// ////////////////////// //

Outcome Texture::resize(const uint32 width, const uint32 height) {
    if (!is_writable()) return Outcome::Failed;
    _width  = width;
    _height = height;
    return Outcome::Successful;
}

} // namespace ENGINE_NAMESPACE