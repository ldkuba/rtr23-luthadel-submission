#include "renderer/render_target.hpp"

namespace ENGINE_NAMESPACE {

// Constructor && Destructor
RenderTarget::RenderTarget(FrameBuffer* const framebuffer, const Config& config)
    : _framebuffer(framebuffer), _sync_mode(config.sync_mode),
      _width(config.width), _height(config.height) {
    add_attachments(config.attachments);
}
RenderTarget::~RenderTarget() {}

// //////////////////////////// //
// RENDER TARGET PUBLIC METHODS //
// //////////////////////////// //

void RenderTarget::resize(const uint32 width, const uint32 height) {
    _width  = width;
    _height = height;

    if (_sync_mode == SynchMode::HalfResolution) {
        _width  = std::max(_width / 2, 1u);
        _height = std::max(_height / 2, 1u);
    }

    for (const auto& att : _attachments)
        att->resize(_width, _height);
    _framebuffer->recreate(_width, _height, _attachments);
}

void RenderTarget::add_attachments(const Vector<Texture*>& attachments) {
    for (const auto& att : attachments)
        _attachments.push_back(att);
}
void RenderTarget::free_attachments() { _attachments.clear(); }

} // namespace ENGINE_NAMESPACE