#include "renderer/views/render_view_skybox.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_VIEW_SKYBOX_LOG "RenderViewSkybox :: "

uint16 get_uniform_location(const Shader* const shader, const String& uniform);

// Constructor & Destructor
RenderViewSkybox::RenderViewSkybox(
    const Config&       config,
    ShaderSystem* const shader_system,
    Camera* const       world_camera
)
    : RenderView(config) {

    // Load shader
    auto res = shader_system->acquire(_shader_name);
    if (res.has_error()) {
        Logger::error(
            RENDER_VIEW_SKYBOX_LOG,
            "Shader `",
            _shader_name,
            "` does not exist. View creation is faulty. For now default "
            "Skybox shader will be used. This could result in some undefined "
            "behaviour."
        );
        res = shader_system->acquire(Shader::BuiltIn::SkyboxShader);
    }
    _shader = res.value();

    // Load uniform locations
    _u_index = { _shader };

    // Set some defaults
    _near_clip = 0.1f;   // TODO: TEMP
    _far_clip  = 1000.f; // TODO: TEMP
    _fov       = glm::radians(45.f);

    // Default value, jic, so that it doesn't fail right away
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );
    _world_camera = world_camera;
}
RenderViewSkybox::~RenderViewSkybox() {}

// ///////////////////////////////// //
// RENDER VIEW SKYBOX PUBLIC METHODS //
// ///////////////////////////////// //

RenderViewPacket RenderViewSkybox::on_build_pocket() {
    return { this,
             _world_camera->transform.position(),
             _world_camera->view(),
             _proj_matrix,
             _shader,
             {} };
}
void RenderViewSkybox::on_resize(const uint32 width, const uint32 height) {
    if (width == _width && height == _height) return;

    _width       = width;
    _height      = height;
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );
}
void RenderViewSkybox::on_render(
    Renderer* const         renderer,
    const RenderViewPacket& packet,
    const uint64            frame_number,
    const uint64            render_target_index
) {
    for (const auto& pass : _passes) {
        // Bind pass
        pass->begin(render_target_index);

        // Setup shader
        _shader->use();

        // Apply globals
        apply_globals(frame_number);

        // Apply instance
        apply_instance(frame_number);

        // Draw geometry
        renderer->draw_geometry(_skybox->geometry());

        // End pass
        pass->end();
    }
}

// //////////////////////////////////// //
// RENDER VIEW SKYBOX PROTECTED METHODS //
// //////////////////////////////////// //

#define uniform_index(uniform)                                                 \
    auto _u_##uniform##_id = shader->get_uniform_index(#uniform);              \
    if (_u_##uniform##_id.has_error()) {                                       \
        Logger::error(                                                         \
            RENDER_VIEW_SKYBOX_LOG, _u_##uniform##_id.error().what()           \
        );                                                                     \
    } else uniform = _u_##uniform##_id.value()

RenderViewSkybox::UIndex::UIndex(const Shader* const shader) {
    uniform_index(projection);
    uniform_index(view);
    uniform_index(cube_texture);
}

#define uniform_set(uniform, value)                                            \
    if (_u_index.uniform != (uint16) -1) {                                     \
        const auto res_##uniform =                                             \
            _shader->set_uniform(_u_index.uniform, &value);                    \
        if (res_##uniform.has_error()) {                                       \
            Logger::error(                                                     \
                RENDER_VIEW_SKYBOX_LOG, res_##uniform.error().what()           \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }

void RenderViewSkybox::apply_globals(const uint64 frame_number) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    // Zero out view position
    auto view_matrix  = _world_camera->view();
    view_matrix[3][0] = 0;
    view_matrix[3][1] = 0;
    view_matrix[3][2] = 0;

    // Apply globals update
    uniform_set(projection, _proj_matrix);
    uniform_set(view, view_matrix);
    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}

void RenderViewSkybox::apply_instance(const uint64 frame_number) const {
    // Apply instance level uniforms
    _shader->bind_instance(_skybox->instance_id);
    _shader->apply_instance();
}

// /////////////////////////////////// //
// RENDER VIEW SKYBOX HELPER FUNCTIONS //
// /////////////////////////////////// //

uint16 get_uniform_location(const Shader* const shader, const String& uniform) {
    auto uniform_res = shader->get_uniform_index(uniform);
    if (uniform_res.has_error()) Logger::fatal();
    return uniform_res.value();
}

} // namespace ENGINE_NAMESPACE