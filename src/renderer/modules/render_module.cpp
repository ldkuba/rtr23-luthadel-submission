#include "renderer/modules/render_module.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_MODULE_LOG "RenderModule :: "

// Constructor & Destructor
RenderModule::RenderModule(
    Renderer* const       renderer,
    ShaderSystem* const   shader_system,
    TextureSystem* const  texture_system,
    GeometrySystem* const geometry_system,
    LightSystem* const    light_system,
    const Config&         config
)
    : _renderer(renderer), _shader_system(shader_system),
      _texture_system(texture_system), _geometry_system(geometry_system),
      _light_system(light_system) {
    // Get render pass
    const auto rp_res = _renderer->get_renderpass(config.render_pass);
    if (rp_res.has_error())
        Logger::fatal(RENDER_MODULE_LOG, rp_res.error().what());
    _renderpass = rp_res.value();

    // Get shader
    const auto sh_res = _shader_system->acquire(config.shader);
    if (sh_res.has_error())
        Logger::fatal(RENDER_MODULE_LOG, sh_res.error().what());
    _shader = sh_res.value();
}
RenderModule::~RenderModule() {
    for (const auto map : _own_maps)
        _renderer->destroy_texture_map(map);
    for (const auto texture : _own_textures)
        _texture_system->release(texture->name);
}

// //////////////////////////// //
// RENDER MODULE PUBLIC METHODS //
// //////////////////////////// //

ModulePacket* RenderModule::build_pocket() { return on_build_pocket(); }

void RenderModule::render(
    const ModulePacket* const packet, const uint64 frame_number
) {
    // Transition when necessary
    for (const auto& map : _own_maps)
        if (map->texture->is_render_target())
            map->texture->transition_render_target(frame_number);

    // Start render pas
    _renderpass->begin();

    // Setup shader
    _shader->use();

    // Apply globals
    apply_globals(frame_number);

    // Perform all calls
    on_render(packet, frame_number);

    // End render pass
    _renderpass->end();
}

// /////////////////////////////// //
// RENDER MODULE PROTECTED METHODS //
// /////////////////////////////// //

ModulePacket* RenderModule::on_build_pocket() {
    return new (MemoryTag::Temp) ModulePacket { this };
}

// ///////////////////////////// //
// RENDER MODULE PRIVATE METHODS //
// ///////////////////////////// //

void RenderModule::apply_globals(uint64 frame_number) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    // Apply individual global uniforms
    apply_globals();

    // Apply globals
    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}

Texture::Map* RenderModule::create_texture_map(
    const String&          texture,
    const Texture::Use&    use,
    const Texture::Filter& filter_minify,
    const Texture::Filter& filter_magnify,
    const Texture::Repeat& repeat_u,
    const Texture::Repeat& repeat_v,
    const Texture::Repeat& repeat_w
) {
    const auto new_texture = _texture_system->acquire(texture, true);
    const auto new_map     = _renderer->create_texture_map({ new_texture,
                                                             use,
                                                             filter_minify,
                                                             filter_magnify,
                                                             repeat_u,
                                                             repeat_v,
                                                             repeat_w });
    _own_textures.push_back(new_texture);
    _own_maps.push_back(new_map);
    return new_map;
}

} // namespace ENGINE_NAMESPACE