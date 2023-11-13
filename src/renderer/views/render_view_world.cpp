#include "renderer/views/render_view_world.hpp"

#include "resources/mesh.hpp"

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
        res = shader_system->acquire(ShaderSystem::BuiltIn::MaterialShader);
    }

    // Setup values
    _shader    = res.value();
    _near_clip = 0.1f;   // TODO: TEMP
    _far_clip  = 1000.f; // TODO: TEMP
    _fov       = glm::radians(45.f);

    // Default value, jic, so that it doesn't fail right away
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );

    _world_camera = world_camera;

    // TODO: Obtain ambient color from the scene
    _ambient_color = glm::vec4(0.25f, 0.25f, 0.25f, 1.f);
}
RenderViewWorld::~RenderViewWorld() {}

// ///////////////////////////// //
// RENDER VIEW UI PUBLIC METHODS //
// ///////////////////////////// //

RenderViewPacket RenderViewWorld::on_build_pocket() {
    // Get geometry data
    static Vector<GeometryRenderData> geometries_data {};
    geometries_data.clear();

    // TODO: Performance :/
    typedef std::pair<float32, GeometryRenderData> TGeomData;

    static Vector<TGeomData> transparent_geometries {};
    transparent_geometries.clear();

    // Check if render data is set
    if (_render_data != nullptr) {
        // Add all opaque geometries
        for (const auto& mesh : _render_data->meshes) {
            const auto model_matrix = mesh->transform.world();
            for (auto* const geom : mesh->geometries()) {
                const GeometryRenderData render_data { geom, model_matrix };
                // TODO: Add something in material to check for transparency.
                if (geom->material()->diffuse_map()->texture->has_transparency(
                    ) == false)
                    geometries_data.push_back(render_data);
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
        std::sort(
            transparent_geometries.begin(),
            transparent_geometries.end(),
            [](const TGeomData& x, const TGeomData& y) -> bool {
                return x.first > y.first;
            }
        );

        // Add all transparent geometries
        for (const auto& t_geom : transparent_geometries)
            geometries_data.push_back(t_geom.second);
    } else {
        Logger::warning(
            RENDER_VIEW_WORLD_LOG,
            "Render data not set for view `",
            _name,
            "`. Not much will be drawn."
        );
    }

    return { this,
             _world_camera->transform.position(),
             _world_camera->view(),
             _proj_matrix,
             _shader,
             geometries_data };
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

        // Draw geometries
        for (const auto& geo_data : packet.geometry_data) {
            // Update material instance
            Material* const geo_material = geo_data.geometry->material;
            geo_material->apply_instance();

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

#define set_uniform(uniform_name, uniform_value)                               \
    {                                                                          \
        auto uniform_id_res = _shader->get_uniform_index(uniform_name);        \
        if (uniform_id_res.has_error()) {                                      \
            Logger::error(                                                     \
                RENDER_VIEW_WORLD_LOG,                                         \
                "Shader set_uniform method failed. No uniform is named \"",    \
                uniform_name,                                                  \
                "\". Nothing was done."                                        \
            );                                                                 \
            return;                                                            \
        }                                                                      \
        auto uniform_id = uniform_id_res.value();                              \
        auto set_result = _shader->set_uniform(uniform_id, &uniform_value);    \
        if (set_result.has_error()) {                                          \
            Logger::error(                                                     \
                RENDER_VIEW_WORLD_LOG,                                         \
                "Shader set_uniform method failed for \"",                     \
                uniform_name,                                                  \
                "\". Nothing was done"                                         \
            );                                                                 \
            return;                                                            \
        }                                                                      \
    }

void RenderViewWorld::apply_globals(const uint64 frame_number) const {
    // Globals can be updated only once per frame
    if (frame_number == _shader->rendered_frame_number) return;

    // Apply globals update
    set_uniform("projection", _proj_matrix);
    set_uniform("view", _world_camera->view());
    set_uniform("ambient_color", _ambient_color);
    set_uniform("view_position", _world_camera->transform.position());
    set_uniform("mode", _render_mode);
    _shader->apply_global();

    // Update render frame number
    _shader->rendered_frame_number = frame_number;
}
void RenderViewWorld::apply_locals(const glm::mat4 model) const {
    set_uniform("model", model);
}

} // namespace ENGINE_NAMESPACE