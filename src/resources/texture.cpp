#include "resources/texture.hpp"
#include <cmath>

namespace ENGINE_NAMESPACE {

#define TEXTURE_LOG "Texture :: "

// Constructor & Destructor
Texture::Texture(const Config& config)
    : _name(config.name), _width(config.width), _height(config.height),
      _channel_count(config.channel_count), _format(config.format),
      _type(config.type), _flags(0) {
    _total_size = width * height * channel_count;
    _mip_levels =
        config.is_mip_mapped
            ? (uint8) std::floor(std::log2(std::max(_width, _height))) + 1
            : 1;
    if (_type == Type::TCube) _total_size *= 6;
    if (config.has_transparency) _flags |= HasTransparency;
    if (config.is_writable) _flags |= IsWritable;
    if (config.is_wrapped) _flags |= IsWrapped;
    if (config.is_render_target) _flags |= IsRenderTarget;
    if (config.is_multisampled) _flags |= IsMultisampled;
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

bool Texture::has_depth_format(Format format) {
    return format == Format::DS24 || format == Format::D32 ||
           format == Format::DS32;
}

// -----------------------------------------------------------------------------
// Packed texture
// -----------------------------------------------------------------------------

// ////////////////////// //
// TEXTURE PUBLIC METHODS //
// ////////////////////// //

Texture* PackedTexture::get_at(uint8 index) const {
    if (index >= _textures.size())
        Logger::fatal(
            TEXTURE_LOG,
            "Attempting to index packed texture passed its final index."
        );
    return _textures[index];
}

void PackedTexture::marked_as_used() {
    Texture::marked_as_used();
    for (auto& texture : _textures)
        texture->marked_as_used();
}

Outcome PackedTexture::write(const byte* const, const uint32, const uint32) {
    Logger::error(
        TEXTURE_LOG,
        "Texture write into attached texture attempted. Textures which are "
        "used as attachments cannot be written to using this function. "
        "Operation failed."
    );
    return Outcome::Failed;
}
Outcome PackedTexture::resize(const uint32 width, const uint32 height) {
    for (auto& texture : _textures) {
        if (texture->resize(width, height).failed()) //
            return Outcome::Failed;
    }
    return Outcome::Successful;
}
Outcome PackedTexture::transition_render_target(const uint64 frame_number) {
    if (frame_number == _last_transition_frame_number) return Outcome::Failed;
    if (_textures[_currently_used_i]
            ->transition_render_target(frame_number)
            .failed())
        return Outcome::Failed;
    _last_transition_frame_number = frame_number;
    _currently_used_i++;
    _currently_used_i %= _textures.size();
    return Outcome::Successful;
}

} // namespace ENGINE_NAMESPACE