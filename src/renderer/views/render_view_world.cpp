#include "renderer/views/render_view_world.hpp"

#include "resources/mesh.hpp"
#include "multithreading/parallel.hpp"
#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

#define RENDER_VIEW_WORLD_LOG "RenderViewWorld :: "

// Constructor & Destructor
RenderViewWorld::RenderViewWorld(
    const Config&       config,
    ShaderSystem* const shader_system,
    Camera* const       world_camera
)
    : RenderView(config) {
    auto res = shader_system->acquire(_shader_name);
    if (res.has_error()) {
        Logger::error(
            RENDER_VIEW_WORLD_LOG,
            "Shader `",
            _shader_name,
            "` does not exist. View creation is faulty. For now default "
            "Material shader will be used. This could result in some undefined "
            "behaviour."
        );
        res = shader_system->acquire(Shader::BuiltIn::MaterialShader);
    }

    // Setup values
    _shader = res.value();

    // Setup shader indices
    _u_index = { _shader };

    _near_clip = 0.1f;   // TODO: TEMP
    _far_clip  = 1000.f; // TODO: TEMP
    _fov       = glm::radians(45.f);

    // Default value, jic, so that it doesn't fail right away
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );

    _world_camera = world_camera;

    // TODO: Obtain ambient color from the scene
    _ambient_color = glm::vec4(0.05f, 0.05f, 0.05f, .5f);
}
RenderViewWorld::~RenderViewWorld() {}

// ///////////////////////////// //
// RENDER VIEW UI PUBLIC METHODS //
// ///////////////////////////// //

RenderView::Packet* RenderViewWorld::on_build_pocket() {
    // Clear geometry data
    _geom_data.clear();

    // TODO: Performance :/
    typedef std::pair<float32, GeometryRenderData> TGeomData;

    static Vector<TGeomData> transparent_geometries {};
    transparent_geometries.clear();

    // Check if render data is set
    if (_render_data == nullptr) {
        Logger::warning(
            RENDER_VIEW_WORLD_LOG,
            "Render data not set for view `",
            _name,
            "`. Not much will be drawn."
        );
        return new (MemoryTag::Temp) Packet { this };
    }

    // Add all opaque geometries
    for (const auto& mesh : _render_data->meshes) {
        const auto model_matrix = mesh->transform.world();
        for (auto* const geom : mesh->geometries()) {
            const GeometryRenderData render_data { geom,
                                                   geom->material,
                                                   model_matrix };
            // TODO: Add something in material to check for transparency.
            if (geom->material()->diffuse_map()->texture->has_transparency() ==
                false)
                _geom_data.push_back(render_data);
            else {
                const auto geom3d   = dynamic_cast<Geometry3D*>(geom);
                const auto center   = geom3d->bbox.get_center();
                const auto distance = glm::distance(
                    _world_camera->transform.position(),
                    { model_matrix * glm::vec4(center, 1) }
                );
                transparent_geometries.push_back({ distance, render_data });
            }
        }
    }

    // Sort transparent geometry list
    Parallel::sort(
        transparent_geometries.begin(),
        transparent_geometries.end(),
        [](const TGeomData& x, const TGeomData& y) -> bool {
            return x.first > y.first;
        }
    );

    // Add all transparent geometries
    for (const auto& t_geom : transparent_geometries)
        _geom_data.push_back(t_geom.second);

    return new (MemoryTag::Temp) Packet { this };
}

void RenderViewWorld::on_resize(const uint32 width, const uint32 height) {
    if (width == _width && height == _height) return;

    _width       = width;
    _height      = height;
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );
}

void RenderViewWorld::on_render(
    Renderer* const     renderer,
    const Packet* const packet,
    const uint64        frame_number,
    const uint64        render_target_index
) {
    // Transition used render targets
    const auto pt_ssao =
        static_cast<const PackedTexture*>(_ssao_texture_map->texture);
    pt_ssao->get_at(frame_number % VulkanSettings::max_frames_in_flight)
        ->transition_render_target(); // TODO: Vulkan agnostic

    const auto pt_shadowmap_directional = static_cast<const PackedTexture*>(
        _shadowmap_directional_texture_map->texture
    );
    pt_shadowmap_directional
        ->get_at(frame_number % VulkanSettings::max_frames_in_flight)
        ->transition_render_target(); // TODO: Vulkan agnostic

    for (const auto& pass : _passes) {
        // Bind pass
        pass->begin(render_target_index);

        // Setup shader
        _shader->use();

        // Apply globals
        apply_globals(frame_number);

        // Draw geometries
        for (const auto& geo_data : _geom_data) {
            // Update material instance
            geo_data.material->apply_instance();

            // Apply local
            apply_locals(geo_data.model);

            // Draw geometry
            renderer->draw_geometry(geo_data.geometry);
        }

        // End pass
        pass->end();
    }
}

// //////////////////////////////// //
// RENDER VIEW UI PROTECTED METHODS //
// //////////////////////////////// //

#define uniform_index(uniform)                                                 \
    auto _u_##uniform##_id = shader->get_uniform_index(#uniform);              \
    if (_u_##uniform##_id.has_error()) {                                       \
        Logger::error(                                                         \
            RENDER_VIEW_WORLD_LOG, _u_##uniform##_id.error().what()            \
        );                                                                     \
    } else uniform = _u_##uniform##_id.value()

RenderViewWorld::UIndex::UIndex(const Shader* const shader) {
    uniform_index(projection);
    uniform_index(view);
    uniform_index(ambient_color);
    uniform_index(view_position);
    uniform_index(mode);
    uniform_index(light_space_directional);
    uniform_index(model);
    uniform_index(directional_light);
    uniform_index(num_point_lights);
    uniform_index(point_lights);
    uniform_index(ssao_texture);
    uniform_index(shadowmap_directional_texture);
}

#define uniform_set(uniform, value)                                            \
    if (_u_index.uniform != (uint16) -1) {                                     \
        const auto res_##uniform =                                             \
            _shader->set_uniform(_u_index.uniform, &value);                    \
        if (res_##uniform.has_error()) {                                       \
            Logger::error(                                                     \
                RENDER_VIEW_WORLD_LOG, res_##uniform.error().what()            \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }
#define sampler_set(sampler, texture_map)                                      \
    if (_u_index.sampler != (uint16) -1) {                                     \
        const auto res_##sampler =                                             \
            _shader->set_sampler(_u_index.sampler, texture_map);               \
        if (res_##sampler.has_error()) {                                       \
            Logger::error(                                                     \
                RENDER_VIEW_WORLD_LOG, res_##sampler.error().what()            \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }

void RenderViewWorld::apply_globals(const uint64 frame_number) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    glm::mat4 light_space_directional =
        _light_system->get_directional()->get_light_space_matrix(
            _world_camera->transform.position()
        );

    // Apply globals update
    uniform_set(projection, _proj_matrix);
    uniform_set(view, _world_camera->view());
    uniform_set(ambient_color, _ambient_color);
    uniform_set(view_position, _world_camera->transform.position());
    uniform_set(mode, _render_mode);
    uniform_set(light_space_directional, light_space_directional);
    sampler_set(ssao_texture, _ssao_texture_map);
    sampler_set(
        shadowmap_directional_texture, _shadowmap_directional_texture_map
    );

    // Apply lights
    auto point_light_data  = _light_system->get_point_data();
    auto directional_light = _light_system->get_directional_data();
    auto num_point_lights  = point_light_data.size();
    uniform_set(directional_light, directional_light);
    uniform_set(num_point_lights, num_point_lights);
    uniform_set(point_lights, *(point_light_data.data()));

    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}
void RenderViewWorld::apply_locals(const glm::mat4 model) const {
    uniform_set(model, model);
}

} // namespace ENGINE_NAMESPACE