#include "renderer/renderer.hpp"

#include "resources/geometry.hpp"

#define RENDERER_LOG "Renderer :: "

Renderer::Renderer(
    const RendererBackendType backend_type, Platform::Surface* const surface
) {
    surface->resize_event.subscribe<Renderer>(this, &Renderer::on_resize);
    switch (backend_type) {
    case Vulkan:
        _backend = new (MemoryTag::Renderer) VulkanBackend(surface);
        return;

    default: break;
    }
}
Renderer::~Renderer() { delete _backend; }

void Renderer::on_resize(const uint32 width, const uint32 height) {
    _projection = glm::perspective(
        glm::radians(45.0f), (float32) width / height, _near_plane, _far_plane
    );
    _backend->resized(width, height);
}
Result<void, RuntimeError> Renderer::draw_frame(const float32 delta_time) {
    _backend->increment_frame_number();

    // Begin frame
    auto result = _backend->begin_frame(delta_time);
    if (result.has_error()) { return {}; }

    // === World shader ===
    _backend->begin_render_pass(BuiltinRenderPass::World);
    material_shader->use();
    Material* current_material = current_geometry->material;

    // Update global state
    glm::mat4 _view = glm::lookAt(
        camera_position,
        camera_position + camera_look_dir,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
    current_material->apply_global(
        _projection, _view, _ambient_color, camera_position, _view_mode
    );

    // Update instances
    current_material->apply_instance();

    // Update locals
    // TODO: Temp code; update one and only object
    static float rotation = 0.0f;
    if (cube_rotation) rotation += 50.0f * delta_time;
    auto model = glm::rotate(
        glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f)
    );
    current_material->apply_local(model);

    // Draw geometry
    GeometryRenderData data = {};
    data.geometry           = current_geometry;
    _backend->draw_geometry(data);

    // End renderpass
    _backend->end_render_pass(BuiltinRenderPass::World);

    // === UI changes ===
    _backend->begin_render_pass(BuiltinRenderPass::UI);
    ui_shader->use();
    Material* ui_material = current_ui_geometry->material;

    // Update global state
    ui_material->apply_global(_projection_ui, _view_ui);

    // Update instance
    ui_material->apply_instance();

    // Update local
    model = glm::mat4(1.0f);
    ui_material->apply_local(model);

    // Draw UI
    data.geometry = current_ui_geometry;
    _backend->draw_geometry(data);

    // End renderpass
    _backend->end_render_pass(BuiltinRenderPass::UI);

    // === END FRAME ===
    result = _backend->end_frame(delta_time);

    if (result.has_error()) {
        // TODO: error handling
        return Failure(result.error());
    }

    return {};
}

void Renderer::create_texture(Texture* texture, const byte* const data) {
    Logger::trace(RENDERER_LOG, "Creating texture.");
    _backend->create_texture(texture, data);
    Logger::trace(RENDERER_LOG, "Texture created.");
}
void Renderer::destroy_texture(Texture* texture) {
    _backend->destroy_texture(texture);
    Logger::trace(RENDERER_LOG, "Texture destroyed.");
}

void Renderer::destroy_geometry(Geometry* geometry) {
    _backend->destroy_geometry(geometry);
    Logger::trace(RENDERER_LOG, "Geometry destroyed.");
}

Shader* Renderer::create_shader(const ShaderConfig config) {
    Logger::trace(RENDERER_LOG, "Creating shader.");
    auto ret = _backend->create_shader(config);
    Logger::trace(RENDERER_LOG, "Shader created.");
    return ret;
}
void Renderer::destroy_shader(Shader* shader) {
    _backend->destroy_shader(shader);
    Logger::trace(RENDERER_LOG, "Shader destroyed.");
}