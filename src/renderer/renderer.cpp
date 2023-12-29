#include "renderer/renderer.hpp"

#include "renderer/views/render_view.hpp"

namespace ENGINE_NAMESPACE {

#define RENDERER_LOG "Renderer :: "

// Constructor & Destructor
Renderer::Renderer(
    const RendererBackend::Type backend_type, Platform::Surface* const surface
) {
    // Setup on resize
    surface->resize_event.subscribe<Renderer>(this, &Renderer::on_resize);

    // Setup backend
    switch (backend_type) {
    case RendererBackend::Type::Vulkan:
        _backend = new (MemoryTag::Renderer) VulkanBackend(surface);
        break;
    default:
        Logger::fatal(
            RENDERER_LOG, "Unimplemented backend passed on initialization."
        );
    }

    // === Render passes ===
    // Get width & height
    const auto width  = surface->get_width_in_pixels();
    const auto height = surface->get_height_in_pixels();

    // Create render passes
    const auto depth_renderpass  = create_render_pass({
        RenderPass::BuiltIn::DepthPass,       // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        true,                                 // Depth testing
        false                                 // Multisampling
    });
    const auto ao_renderpass     = create_render_pass({
        RenderPass::BuiltIn::AOPass,          // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        false,                                // Depth testing
        false                                 // Multisampling
    });
    const auto skybox_renderpass = create_render_pass({
        RenderPass::BuiltIn::SkyboxPass,      // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        false,                                // Depth testing
        true                                  // Multisampling
    });
    const auto world_renderpass  = create_render_pass({
        RenderPass::BuiltIn::WorldPass,       // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        true,                                 // Depth testing
        true                                  // Multisampling
    });
    const auto ui_renderpass     = create_render_pass({
        RenderPass::BuiltIn::UIPass,          // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        false,                                // Depth testing
        false                                 // Multisampling
    });

    // Create render target textures
    // const auto depth_normals_texture = _texture_system.acquire_writable(
    //     "DepthPrePassTarget", width, height, 4, true
    // );

    // Create render targets
    world_renderpass->add_window_as_render_target();
    ui_renderpass->add_window_as_render_target();
    skybox_renderpass->add_window_as_render_target();
    ao_renderpass->add_window_as_render_target();
    depth_renderpass->add_window_as_render_target();
    // depth_renderpass->add_render_target(RenderTarget::Config {
    //     width,
    //     height,
    //     { depth_normals_texture, _app_renderer.get_depth_texture() },
    //     true });

    // Initialize render passes (BASIC)
    // RenderPass::start >> "C" >> skybox_renderpass >> "DS" >> world_renderpass
    // >>
    //     ui_renderpass >> RenderPass::finish;

    // Initialize no skybox basic rp
    // RenderPass::start >> "CDS" >> world_renderpass >> ui_renderpass >>
    //     RenderPass::finish;

    // Initialize render passes
    RenderPass::start >> "DSC" >> depth_renderpass >> "C" >>
        skybox_renderpass >> "DS" >> world_renderpass >> ui_renderpass >>
        RenderPass::finish;

    // Initialize AO only
    // RenderPass::start >> "DS" >> depth_renderpass >> "C" >> ao_renderpass >>
    //     RenderPass::finish;

    // RenderPass::start >> "CDS" >> depth_renderpass >> RenderPass::finish;
}
Renderer::~Renderer() { del(_backend); }

// /////////////////////// //
// RENDERER PUBLIC METHODS //
// /////////////////////// //

Result<void, RuntimeError> Renderer::draw_frame(
    const Packet* const render_data, const float32 delta_time
) {
    _backend->increment_frame_number();

    // Begin frame
    auto result = _backend->begin_frame(delta_time);
    if (result.has_error()) { return {}; }

    // Get current window att index
    const auto att_index = _backend->get_current_window_attachment_index();

    // Render each view
    for (auto& data : render_data->view_data)
        data->view->on_render(
            this, data, _backend->get_current_frame(), att_index
        );

    // Clear render view packets in reverse order
    for (int32 i = render_data->view_data.size() - 1; i >= 0; i--)
        del(render_data->view_data[i]);

    // End frame
    result = _backend->end_frame(delta_time);

    if (result.has_error()) {
        // TODO: error handling
        return Failure(result.error());
    }

    return {};
}

void Renderer::draw_geometry(Geometry* const geometry) {
    _backend->draw_geometry(geometry);
}

void Renderer::on_resize(const uint32 width, const uint32 height) {
    _backend->resized(width, height);
}

// -----------------------------------------------------------------------------
// Texture
// -----------------------------------------------------------------------------

Texture* Renderer::create_texture(
    const Texture::Config& config, const byte* const data
) {
    Logger::trace(RENDERER_LOG, "Creating texture.");
    const auto texture = _backend->create_texture(config, data);
    Logger::trace(RENDERER_LOG, "Texture created [", texture->name(), "].");
    return texture;
}
Texture* Renderer::create_writable_texture(const Texture::Config& config) {
    Logger::trace(RENDERER_LOG, "Creating writable texture.");
    const auto texture = _backend->create_writable_texture(config);
    Logger::trace(
        RENDERER_LOG, "Writable texture created [`", texture->name(), "`]."
    );
    return texture;
}
void Renderer::destroy_texture(Texture* texture) {
    _backend->destroy_texture(texture);
    Logger::trace(RENDERER_LOG, "Texture destroyed [`", texture->name(), "`].");
}

// -----------------------------------------------------------------------------
// Texture map
// -----------------------------------------------------------------------------

Texture::Map* Renderer::create_texture_map(const Texture::Map::Config& config) {
    Logger::trace(RENDERER_LOG, "Creating texture map.");
    const auto map = _backend->create_texture_map(config);
    Logger::trace(
        RENDERER_LOG,
        "Texture map created [",
        map->texture->name(),
        " - ",
        (uint32) map->use,
        "]."
    );
    return map;
}
void Renderer::destroy_texture_map(Texture::Map* map) {
    _backend->destroy_texture_map(map);
    Logger::trace(
        RENDERER_LOG,
        "Texture map destroyed [`",
        map->texture->name(),
        " - ",
        (uint32) map->use,
        "`]."
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

Shader* Renderer::create_shader(const Shader::Config& config) {
    Logger::trace(RENDERER_LOG, "Creating shader.");
    auto ret = _backend->create_shader(_texture_system, config);
    Logger::trace(RENDERER_LOG, "Shader created [", config.name(), "].");
    return ret;
}
void Renderer::destroy_shader(Shader* shader) {
    _backend->destroy_shader(shader);
    Logger::trace(RENDERER_LOG, "Shader destroyed [", shader->get_name(), "].");
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
// Default textures
// -----------------------------------------------------------------------------

Texture* Renderer::get_depth_texture() const {
    return _backend->get_depth_attachment();
}
Texture* Renderer::get_color_texture() const {
    return _backend->get_color_attachment();
}

} // namespace ENGINE_NAMESPACE