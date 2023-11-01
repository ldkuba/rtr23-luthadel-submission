#include "renderer/render_target.hpp"

namespace ENGINE_NAMESPACE {

// Constructor && Destructor
RenderTarget::RenderTarget(
    const Vector<Texture*>& attachments,
    FrameBuffer* const      framebuffer,
    const uint32            width,
    const uint32            height
)
    : _framebuffer(framebuffer), _sync_to_window_resize(true), //
      _width(width), _height(height) {
    add_attachments(attachments);
}
RenderTarget::~RenderTarget() {}

// //////////////////////////// //
// RENDER TARGET PUBLIC METHODS //
// //////////////////////////// //

void RenderTarget::resize(const uint32 width, const uint32 height) {
    _width  = width;
    _height = height;
    _framebuffer->recreate(width, height, _attachments);
}

void RenderTarget::add_attachments(const Vector<Texture*>& attachments) {
    for (const auto& att : attachments)
        _attachments.push_back(att);
}
void RenderTarget::free_attachments() {
    for (const auto& att : _attachments)
        del(att);
    _attachments.clear();
}

} // namespace ENGINE_NAMESPACE