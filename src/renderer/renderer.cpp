#include "renderer/renderer.hpp"

Renderer::Renderer(RendererBackendType backend_type, Platform::Surface* surface) {
    switch (backend_type) {
    case Vulkan:
        _backend = new VulkanBackend(surface);
        return;

    default:
        break;
    }
}
Renderer::~Renderer() {}

void Renderer::on_resize(uint32 width, uint32 height) {}
bool Renderer::draw_frame(float32 delta_time) {
    if (_backend->begin_frame(delta_time)) {
        bool result = _backend->end_frame(delta_time);
        _backend->increment_frame_number();
        if (!result) {
            // TODO: error handling
            return false;
        }
    }

    return true;
}

void Renderer::wait_for_shutdown() {
    _backend->wait_for_shutdown();
}