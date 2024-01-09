#include "renderer/renderer.hpp"

#include "renderer/modules/render_module.hpp"
#include "timer.hpp"

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
}
Renderer::~Renderer() { del(_backend); }

// /////////////////////// //
// RENDERER PUBLIC METHODS //
// /////////////////////// //

Result<void, RuntimeError> Renderer::draw_frame(
    const Packet* const render_data, const float32 delta_time
) {
    _backend->increment_frame_number();

    // Timer
    Timer& timer = Timer::global_timer;

    // Begin frame
    auto result = _backend->begin_frame(delta_time);
    if (result.has_error()) { return {}; }

    // Get current window att index
    const auto att_index = _backend->get_current_window_attachment_index();

    timer.time("Frame begun in ");

    // Render each module
    for (const auto& data : render_data->module_data)
        data->module->render(data, _backend->get_current_frame());

    // Clear render module packets in reverse order
    for (int32 i = render_data->module_data.size() - 1; i >= 0; i--)
        del(render_data->module_data[i]);

    timer.time("On render preformed in ");

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

void Renderer::viewport_set(glm::vec4 rect) { _backend->viewport_set(rect); }

void Renderer::viewport_reset() { _backend->viewport_reset(); }

void Renderer::scissors_set(glm::vec4 rect) { _backend->scissors_set(rect); }

void Renderer::scissors_reset() { _backend->scissors_reset(); }

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
Texture* Renderer::get_ms_depth_texture() const {
    return _backend->get_ms_depth_attachment();
}
Texture* Renderer::get_ms_color_texture() const {
    return _backend->get_ms_color_attachment();
}

} // namespace ENGINE_NAMESPACE