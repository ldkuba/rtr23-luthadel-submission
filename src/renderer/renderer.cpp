#include "renderer/renderer.hpp"

namespace ENGINE_NAMESPACE {

#define RENDERER_LOG "Renderer :: "

// Constructor & Destructor
Renderer::Renderer(
    const RendererBackendType backend_type, Platform::Surface* const surface
) {
    // Setup on resize
    surface->resize_event.subscribe<Renderer>(this, &Renderer::on_resize);

    // Setup backend
    switch (backend_type) {
    case Vulkan:
        _backend = new (MemoryTag::Renderer) VulkanBackend(surface);
        break;
    default:
        Logger::fatal(
            RENDERER_LOG, "Unimplemented backend passed on initialization."
        );
    }

    // Setup render passes TODO: CONFIGURABLE
    const String WORLD  = "Renderpass.Builtin.World";
    const String UI     = "Renderpass.Builtin.UI";
    const auto   width  = surface->get_width_in_pixels();
    const auto   height = surface->get_height_in_pixels();

    // Create render passes
    _world_renderpass = _backend->create_render_pass({
        WORLD,                                // Name
        "",                                   // Prev
        UI,                                   // Next
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        RenderPass::ClearFlags::Color | RenderPass::ClearFlags::Depth |
            RenderPass::ClearFlags::Stencil, // Clear flags
        true,                                // Depth testing
        true                                 // Multisampling
    });
    _ui_renderpass    = _backend->create_render_pass({
        UI,                                   // Name
        WORLD,                                // Prev
        "",                                   // Next
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        RenderPass::ClearFlags::None,         // Clear flags
        false,                                // Depth testing
        false                                 // Multisampling
    });

    // Create render targets
    const auto count = _backend->get_window_attachment_count();
    for (uint8 i = 0; i < count; i++) {
        // Gather render target attachments
        Vector<Texture*> attachments {};
        attachments.push_back(_backend->get_color_attachment());
        attachments.push_back(_backend->get_depth_attachment());
        attachments.push_back(_backend->get_window_attachment(i));
        // Add new render target
        _world_renderpass->add_render_target(
            create_render_target(_world_renderpass, width, height, attachments)
        );
        _ui_renderpass->add_render_target(create_render_target(
            _ui_renderpass,
            width,
            height,
            { _backend->get_window_attachment(i) }
        ));
    }
}
Renderer::~Renderer() { del(_backend); }

// /////////////////////// //
// RENDERER PUBLIC METHODS //
// /////////////////////// //

Result<void, RuntimeError> Renderer::draw_frame(
    const RenderPacket* const render_data, const float32 delta_time
) {
    _backend->increment_frame_number();

    // Begin frame
    auto result = _backend->begin_frame(delta_time);
    if (result.has_error()) { return {}; }

    // Get current window att index
    const auto att_index = _backend->get_current_window_attachment_index();

    // === World shader ===
    // Bind render pass
    _world_renderpass->begin(att_index);
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
    _world_renderpass->end();

    // === UI changes ===
    _ui_renderpass->begin(att_index);
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
    _ui_renderpass->end();

    // === END FRAME ===
    result = _backend->end_frame(delta_time);

    if (result.has_error()) {
        // TODO: error handling
        return Failure(result.error());
    }

    return {};
}

void Renderer::on_resize(const uint32 width, const uint32 height) {
    // Update projection
    _projection = glm::perspective(
        glm::radians(45.0f), (float32) width / height, _near_plane, _far_plane
    );
    _projection_ui = glm::ortho(
        0.0f, (float32) width, (float32) height, 0.0f, -100.0f, 100.0f
    );

    // Update backend
    _backend->resized(width, height);
}

// -----------------------------------------------------------------------------
// Texture
// -----------------------------------------------------------------------------

void Renderer::create_texture(Texture* texture, const byte* const data) {
    Logger::trace(RENDERER_LOG, "Creating texture.");
    _backend->create_texture(texture, data);
    Logger::trace(RENDERER_LOG, "Texture created [", texture->name(), "].");
}
void Renderer::create_writable_texture(Texture* texture) {
    Logger::trace(RENDERER_LOG, "Creating writable texture.");
    _backend->create_writable_texture(texture);
    Logger::trace(
        RENDERER_LOG, "Writable texture created [`", texture->name(), "`]."
    );
}
void Renderer::destroy_texture(Texture* texture) {
    _backend->destroy_texture(texture);
    Logger::trace(RENDERER_LOG, "Texture destroyed [`", texture->name(), "`].");
}

void Renderer::resize_texture(
    Texture* const texture, const uint32 width, const uint32 height
) {
    Logger::trace(RENDERER_LOG, "Resizing texture [", texture->name(), "].");
    _backend->resize_texture(texture, width, height);
    Logger::trace(RENDERER_LOG, "Texture resized [", texture->name(), "].");
}
void Renderer::texture_write_data(
    Texture* const texture, const Vector<byte>& data, const uint32 offset
) {
    Logger::trace(
        RENDERER_LOG, "Writing data to texture [", texture->name(), "]."
    );
    _backend->texture_write_data(texture, data.data(), data.size(), offset);
    Logger::trace(
        RENDERER_LOG, "Texture writing complete [", texture->name(), "]."
    );
}
void Renderer::texture_write_data(
    Texture* const    texture,
    const byte* const data,
    const uint32      size,
    const uint32      offset
) {
    Logger::trace(
        RENDERER_LOG, "Writing data to texture [", texture->name(), "]."
    );
    _backend->texture_write_data(texture, data, size, offset);
    Logger::trace(
        RENDERER_LOG, "Texture writing complete [", texture->name(), "]."
    );
}

// -----------------------------------------------------------------------------
// Geometry
// -----------------------------------------------------------------------------

void Renderer::destroy_geometry(Geometry* geometry) {
    _backend->destroy_geometry(geometry);
    Logger::trace(RENDERER_LOG, "Geometry destroyed [", geometry->name(), "].");
}

// -----------------------------------------------------------------------------
// Shader
// -----------------------------------------------------------------------------

Shader* Renderer::create_shader(const ShaderConfig config) {
    Logger::trace(RENDERER_LOG, "Creating shader.");
    auto ret = _backend->create_shader(config);
    Logger::trace(RENDERER_LOG, "Shader created [", config.name(), "].");
    return ret;
}
void Renderer::destroy_shader(Shader* shader) {
    _backend->destroy_shader(shader);
    Logger::trace(RENDERER_LOG, "Shader destroyed [", shader->get_name(), "].");
}

// -----------------------------------------------------------------------------
// Render target
// -----------------------------------------------------------------------------

RenderTarget* Renderer::create_render_target(
    RenderPass* const       pass,
    const uint32            width,
    const uint32            height,
    const Vector<Texture*>& attachments
) {
    Logger::trace(RENDERER_LOG, "Creating render target.");
    const auto res =
        _backend->create_render_target(pass, width, height, attachments);
    Logger::trace(RENDERER_LOG, "Render target created.");
    return res;
}
void Renderer::destroy_render_target(
    RenderTarget* const render_target, const bool free_internal_data
) {
    _backend->destroy_render_target(render_target, free_internal_data);
    Logger::trace(RENDERER_LOG, "Render target destroyed.");
}

// -----------------------------------------------------------------------------
// Render pass
// -----------------------------------------------------------------------------

RenderPass* Renderer::create_render_pass(const RenderPass::Config& config) {
    Logger::trace(RENDERER_LOG, "Creating render pass.");
    const auto res = _backend->create_render_pass(config);
    Logger::trace(
        RENDERER_LOG,
        "Render target created [",
        res->id(),
        " (",
        config.name,
        ")]."
    );
    return res;
}
void Renderer::destroy_render_pass(RenderPass* const pass) {
    _backend->destroy_render_pass(pass);
    Logger::trace(RENDERER_LOG, "Render pass destroyed [", pass->id(), "].");
}
Result<RenderPass*, RuntimeError> Renderer::get_renderpass(const String& name) {
    return _backend->get_render_pass(name);
}

// -----------------------------------------------------------------------------
// Camera
// -----------------------------------------------------------------------------

void Renderer::set_active_camera(Camera* const camera) {
    _active_camera = camera;
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

    // Apply globals
    set_uniform("projection", _projection);
    set_uniform("view", _active_camera->view());
    set_uniform("ambient_color", _ambient_color);
    set_uniform("view_position", _active_camera->transform.position());
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