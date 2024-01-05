#include "renderer/views/render_view_blur.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_VIEW_BLUR_LOG "RenderViewBlur :: "

// Constructor & Destructor
RenderViewBlur::RenderViewBlur(
    const Config&         config,
    Renderer* const       renderer,
    TextureSystem* const  texture_system,
    GeometrySystem* const geometry_system,
    ShaderSystem* const   shader_system
)
    : RenderView(config), _renderer(renderer), _texture_system(texture_system) {
    auto res = shader_system->acquire(_shader_name);
    if (res.has_error()) {
        Logger::error(
            RENDER_VIEW_BLUR_LOG,
            "Shader `",
            _shader_name,
            "` does not exist. View creation is faulty. For now default "
            "Blur shader will be used. This could result in some undefined "
            "behaviour."
        );
        res = shader_system->acquire(Shader::BuiltIn::BlurShader);
    }

    // Setup values
    _shader = res.value();

    // Setup shader indices
    _u_index = { _shader };

    // Create full screen render packet
    // Size is [2, 2], since shader converts positions: [0, 2] -> [-1, 1]
    _full_screen_geometry =
        geometry_system->generate_ui_rectangle("full_screen_geom", 2, 2);

    //  Create texture maps
    // Depth texture
    const auto ssao_texture = _texture_system->acquire("SSAOPassTarget", false);
    _ssao_map               = _renderer->create_texture_map({ ssao_texture,
                                                              Texture::Use::Unknown,
                                                              Texture::Filter::BiLinear,
                                                              Texture::Filter::BiLinear,
                                                              Texture::Repeat::ClampToEdge,
                                                              Texture::Repeat::ClampToEdge,
                                                              Texture::Repeat::ClampToEdge });
}
RenderViewBlur::~RenderViewBlur() {
    if (_ssao_map) _renderer->destroy_texture_map(_ssao_map);
}

// /////////////////////////////// //
// RENDER VIEW BLUR PUBLIC METHODS //
// /////////////////////////////// //

RenderView::Packet* RenderViewBlur::on_build_pocket() {
    return new (MemoryTag::Temp) Packet { this };
}

void RenderViewBlur::on_resize(const uint32 width, const uint32 height) {
    const auto half_width  = std::max<uint32>(width / 2, 1);
    const auto half_height = std::max<uint32>(height / 2, 1);

    if (half_width == _width && half_height == _height) return;

    _width  = half_width;
    _height = half_height;

    for (const auto& pass : _passes) {
        for (const auto& target : pass->render_targets())
            target->resize(half_width, half_height);
    }
}

void RenderViewBlur::on_render(
    Renderer* const     renderer,
    const Packet* const packet,
    const uint64        frame_number,
    const uint64        render_target_index
) {
    // Transition used render targets
    const auto pt = static_cast<const PackedTexture*>(_ssao_map->texture);
    pt->get_at(frame_number % VulkanSettings::max_frames_in_flight)
        ->transition_render_target(); // TODO: Vulkan agnostic

    for (const auto& pass : _passes) {
        // Bind pass
        pass->begin(frame_number % VulkanSettings::max_frames_in_flight);
        // pass->begin(render_target_index);

        // Setup shader
        _shader->use();

        // Apply globals
        apply_globals(frame_number);

        // Draw 1 geometry
        renderer->draw_geometry(_full_screen_geometry);

        // End pass
        pass->end();
    }
}

// ////////////////////////////////// //
// RENDER VIEW BLUR PROTECTED METHODS //
// ////////////////////////////////// //

#define uniform_index(uniform)                                                 \
    auto _u_##uniform##_id = shader->get_uniform_index(#uniform);              \
    if (_u_##uniform##_id.has_error()) {                                       \
        Logger::error(RENDER_VIEW_BLUR_LOG, _u_##uniform##_id.error().what()); \
    } else uniform = _u_##uniform##_id.value()

RenderViewBlur::UIndex::UIndex(const Shader* const shader) {
    uniform_index(target_texture);
}

#define uniform_set(uniform, value)                                            \
    if (_u_index.uniform != (uint16) -1) {                                     \
        const auto res_##uniform =                                             \
            _shader->set_uniform(_u_index.uniform, &value);                    \
        if (res_##uniform.has_error()) {                                       \
            Logger::error(RENDER_VIEW_BLUR_LOG, res_##uniform.error().what()); \
            return;                                                            \
        }                                                                      \
    }
#define sampler_set(sampler, texture_map)                                      \
    if (_u_index.sampler != (uint16) -1) {                                     \
        const auto res_##sampler =                                             \
            _shader->set_sampler(_u_index.sampler, texture_map);               \
        if (res_##sampler.has_error()) {                                       \
            Logger::error(RENDER_VIEW_BLUR_LOG, res_##sampler.error().what()); \
            return;                                                            \
        }                                                                      \
    }

void RenderViewBlur::apply_globals(const uint64 frame_number) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    // Apply globals update
    sampler_set(target_texture, _ssao_map);
    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}

} // namespace ENGINE_NAMESPACE