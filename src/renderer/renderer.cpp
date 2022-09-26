#include "renderer/renderer.hpp"

Renderer::Renderer(RendererBackendType backend_type, Platform::Surface* surface) {
    surface->resize_event.subscribe<Renderer>(this, &Renderer::on_resize);
    switch (backend_type) {
    case Vulkan:
        _backend = new VulkanBackend(surface);
        return;

    default:
        break;
    }
}
Renderer::~Renderer() {}

void Renderer::on_resize(const uint32 width, const uint32 height) {
    _projection = glm::perspective(glm::radians(45.0f), (float32) width / height, _near_plane, _far_plane);
    _backend->resized(width, height);
}
bool Renderer::draw_frame(const float32 delta_time) {
    if (_backend->begin_frame(delta_time)) {
        _backend->update_global_uniform_buffer_state(
            _projection,
            _view,
            glm::vec3(0.0),
            glm::vec4(1.0),
            0
        );
        bool result = _backend->end_frame(delta_time);
        _backend->increment_frame_number();
        if (!result) {
            // TODO: error handling
            return false;
        }
    }

    return true;
}

void Renderer::create_texture(Texture* texture) {
    _backend->create_texture(texture);
}
void Renderer::destroy_texture(Texture* texture) {
    _backend->destroy_texture(texture);
}