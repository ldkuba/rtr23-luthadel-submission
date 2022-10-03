#include "renderer/renderer.hpp"

#define RENDERER_LOG "Renderer :: "

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
        // Update global state
        _backend->update_global_state(
            _projection,
            _view,
            glm::vec3(0.0),
            glm::vec4(1.0),
            0
        );

        // TODO: Temp code; update one and only object
        static float rotation = 0.0f;
        rotation += 50.0f * delta_time;
        GeometryRenderData data = {};
        data.model = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        data.material = current_material;

        _backend->update_object(data);

        bool result = _backend->end_frame(delta_time);
        _backend->increment_frame_number();
        if (!result) {
            // TODO: error handling
            return false;
        }
    }

    return true;
}

void Renderer::create_texture(Texture* texture, const byte* const data) {
    Logger::trace(RENDERER_LOG, "Creating texture.");
    _backend->create_texture(texture, data);
    Logger::trace(RENDERER_LOG, "Texture created.");
}
void Renderer::destroy_texture(Texture* texture) {
    _backend->destroy_texture(texture);
    Logger::trace(RENDERER_LOG, "Destroying texture.");
}

void Renderer::create_material(Material* const material) {
    Logger::trace(RENDERER_LOG, "Creating material.");
    _backend->create_material(material);
    Logger::trace(RENDERER_LOG, "Material created.");
}
void Renderer::destroy_material(Material* const material) {
    _backend->destroy_material(material);
    Logger::trace(RENDERER_LOG, "Destroying material.");
}