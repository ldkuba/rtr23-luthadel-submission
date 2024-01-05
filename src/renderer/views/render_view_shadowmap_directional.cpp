#include "renderer/views/render_view_shadowmap_directional.hpp"

#include "resources/mesh.hpp"
#include "multithreading/parallel.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_VIEW_SHADOWMAP_DIRECTIONAL_LOG                                  \
    "RenderViewShadowmapDirectional :: "

// Constructor & Destructor
RenderViewShadowmapDirectional::RenderViewShadowmapDirectional(
    const Config&       config,
    ShaderSystem* const shader_system,
    Camera* const       world_camera
)
    : RenderView(config) {
    auto res = shader_system->acquire(_shader_name);
    if (res.has_error()) {
        Logger::error(
            RENDER_VIEW_SHADOWMAP_DIRECTIONAL_LOG,
            "Shader `",
            _shader_name,
            "` does not exist. View creation is faulty. For now default "
            "Shadowmap Directional shader will be used. This could result in "
            "some undefined "
            "behaviour."
        );
        res =
            shader_system->acquire(Shader::BuiltIn::ShadowmapDirectionalShader);
    }

    // Setup values
    _shader = res.value();

    // Setup shader indices
    _u_index = { _shader };
    _world_camera = world_camera;
}
RenderViewShadowmapDirectional::~RenderViewShadowmapDirectional() {}

// ///////////////////////////// //
// RENDER VIEW UI PUBLIC METHODS //
// ///////////////////////////// //

RenderView::Packet* RenderViewShadowmapDirectional::on_build_pocket() {
    // Clear geometry data
    _geom_data.clear();

    // Check if render data is set
    if (_render_data == nullptr) {
        Logger::warning(
            RENDER_VIEW_SHADOWMAP_DIRECTIONAL_LOG,
            "Render data not set for view `",
            _name,
            "`. Not much will be drawn."
        );
        return new (MemoryTag::Temp) Packet { this };
    }

    // Add all geometries
    for (const auto& mesh : _render_data->meshes) {
        const auto model_matrix = mesh->transform.world();
        for (auto* const geom : mesh->geometries()) {
            const GeometryRenderData render_data { geom,
                                                   geom->material,
                                                   model_matrix };
            _geom_data.push_back(render_data);
        }
    }

    return new (MemoryTag::Temp) Packet { this };
}

void RenderViewShadowmapDirectional::on_resize(
    const uint32 width, const uint32 height
) {}

void RenderViewShadowmapDirectional::on_render(
    Renderer* const     renderer,
    const Packet* const packet,
    const uint64        frame_number,
    const uint64        render_target_index
) {
    for (const auto& pass : _passes) {
     
        // Set viewport and scissors
        renderer->viewport_set({0.0f, 0.0f, _width, _height});
        renderer->scissors_set({0.0f, 0.0f, _width, _height});
     
        // Bind pass
        // TODO: Vulkan agnostic way of indexing frames in flight
        pass->begin(frame_number % VulkanSettings::max_frames_in_flight);
        // pass->begin(render_target_index);

        // Setup shader
        _shader->use();

        // Apply globals
        apply_globals(frame_number);

        // Draw geometries
        for (const auto& geo_data : _geom_data) {
            // Apply local
            apply_locals(geo_data.model);

            // Draw geometry
            renderer->draw_geometry(geo_data.geometry);
        }

        // End pass
        pass->end();

        // Reset viewport and scissors
        renderer->viewport_reset();
        renderer->scissors_reset();
    }
}

// //////////////////////////////// //
// RENDER VIEW UI PROTECTED METHODS //
// //////////////////////////////// //

#define uniform_index(uniform)                                                 \
    auto _u_##uniform##_id = shader->get_uniform_index(#uniform);              \
    if (_u_##uniform##_id.has_error()) {                                       \
        Logger::error(                                                         \
            RENDER_VIEW_SHADOWMAP_DIRECTIONAL_LOG,                             \
            _u_##uniform##_id.error().what()                                   \
        );                                                                     \
    } else uniform = _u_##uniform##_id.value()

RenderViewShadowmapDirectional::UIndex::UIndex(const Shader* const shader) {
    uniform_index(light_space);
    uniform_index(model);
}

#define uniform_set(uniform, value)                                            \
    if (_u_index.uniform != (uint16) -1) {                                     \
        const auto res_##uniform =                                             \
            _shader->set_uniform(_u_index.uniform, &value);                    \
        if (res_##uniform.has_error()) {                                       \
            Logger::error(                                                     \
                RENDER_VIEW_SHADOWMAP_DIRECTIONAL_LOG,                         \
                res_##uniform.error().what()                                   \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }

void RenderViewShadowmapDirectional::apply_globals(const uint64 frame_number
) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    // Apply globals update
    glm::mat4 light_space = _light_system->get_directional()->get_light_space_matrix(_world_camera->transform.position());
    uniform_set(light_space, light_space);

    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}
void RenderViewShadowmapDirectional::apply_locals(const glm::mat4 model) const {
    uniform_set(model, model);
}

} // namespace ENGINE_NAMESPACE