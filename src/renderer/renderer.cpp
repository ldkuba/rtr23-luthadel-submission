#include "renderer/renderer.hpp"

#include "resources/geometry.hpp"

namespace ENGINE_NAMESPACE {

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
Renderer::~Renderer() { del(_backend); }

void Renderer::on_resize(const uint32 width, const uint32 height) {
    _projection = glm::perspective(
        glm::radians(45.0f), (float32) width / height, _near_plane, _far_plane
    );
    _backend->resized(width, height);
}
Result<void, RuntimeError> Renderer::draw_frame(
    const RenderPacket* const render_data, const float32 delta_time
) {
    _backend->increment_frame_number();

    // Begin frame
    auto result = _backend->begin_frame(delta_time);
    if (result.has_error()) { return {}; }

    // === World shader ===
    // Bind render pass
    _backend->begin_render_pass(BuiltinRenderPass::World);
    // Use shader
    material_shader->use();
    // Setup shader globals
    update_material_shader_globals();

    // Draw geometries
    for (const auto& geo_data : render_data->geometry_data) {
        // Update material instance
        Material* const geo_material = geo_data.geometry->material;
        geo_material->apply_instance();

        // Apply local
        update_material_shader_locals(geo_data.model);

        // Draw geometry
        _backend->draw_geometry(geo_data.geometry);
    }

    // End renderpass
    _backend->end_render_pass(BuiltinRenderPass::World);

    // === UI changes ===
    _backend->begin_render_pass(BuiltinRenderPass::UI);
    ui_shader->use();
    update_ui_shader_globals();

    // Get UI geometry
    const auto ui_geo = render_data->ui_geometry_data;

    // Update instance
    Material* ui_material = ui_geo.geometry->material;
    ui_material->apply_instance();

    // Update local
    update_ui_shader_locals(ui_geo.model);

    // Draw UI
    _backend->draw_geometry(ui_geo.geometry);

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

void Renderer::create_writable_texture(Texture* texture) {
    Logger::trace(RENDERER_LOG, "Creating writable texture.");
    _backend->create_writable_texture(texture);
    Logger::trace(RENDERER_LOG, "Writable texture created.");
}
void Renderer::resize_texture(
    Texture* const texture, const uint32 width, const uint32 height
) {
    Logger::trace(RENDERER_LOG, "Resizing texture.");
    _backend->resize_texture(texture, width, height);
    Logger::trace(RENDERER_LOG, "Texture resized.");
}
void Renderer::texture_write_data(
    Texture* const texture, const Vector<byte>& data, const uint32 offset
) {
    Logger::trace(RENDERER_LOG, "Writing data to texture.");
    _backend->texture_write_data(texture, data.data(), data.size(), offset);
    Logger::trace(RENDERER_LOG, "Texture writing complete.");
}

void Renderer::texture_write_data(
    Texture* const    texture,
    const byte* const data,
    const uint32      size,
    const uint32      offset
) {
    Logger::trace(RENDERER_LOG, "Writing data to texture.");
    _backend->texture_write_data(texture, data, size, offset);
    Logger::trace(RENDERER_LOG, "Texture writing complete.");
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

// //////////////////////// //
// RENDERER PRIVATE METHODS //
// //////////////////////// //

#define set_uniform(uniform_name, uniform_value)                               \
    {                                                                          \
        auto uniform_id_res = shader->get_uniform_index(uniform_name);         \
        if (uniform_id_res.has_error()) {                                      \
            Logger::error(                                                     \
                RENDERER_LOG,                                                  \
                "Shader set_uniform method failed. No uniform is named \"",    \
                uniform_name,                                                  \
                "\". Nothing was done."                                        \
            );                                                                 \
            return;                                                            \
        }                                                                      \
        auto uniform_id = uniform_id_res.value();                              \
        auto set_result = shader->set_uniform(uniform_id, &uniform_value);     \
        if (set_result.has_error()) {                                          \
            Logger::error(                                                     \
                RENDERER_LOG,                                                  \
                "Shader set_uniform method failed for \"",                     \
                uniform_name,                                                  \
                "\". Nothing was done"                                         \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }

void Renderer::update_material_shader_globals() const {
    const auto shader = material_shader;

    // Compute view matrix
    glm::mat4 view = glm::lookAt(
        camera_position,
        camera_position + camera_look_dir,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    // Apply globals
    set_uniform("projection", _projection);
    set_uniform("view", view);
    set_uniform("ambient_color", _ambient_color);
    set_uniform("view_position", camera_position);
    set_uniform("mode", _view_mode);
    shader->apply_global();
}
void Renderer::update_ui_shader_globals() const {
    const auto shader = ui_shader;

    // Apply globals
    set_uniform("projection", _projection_ui);
    set_uniform("view", _view_ui);
    shader->apply_global();
}
void Renderer::update_material_shader_locals(const glm::mat4 model) const {
    const auto shader = material_shader;
    set_uniform("model", model);
}
void Renderer::update_ui_shader_locals(const glm::mat4 model) const {
    const auto shader = ui_shader;
    set_uniform("model", model);
}

} // namespace ENGINE_NAMESPACE