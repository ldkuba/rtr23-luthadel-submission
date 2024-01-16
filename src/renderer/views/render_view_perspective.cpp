#include "renderer/views/render_view_perspective.hpp"

#include "multithreading/parallel.hpp"
#include "component/frustum.hpp"
#include "resources/mesh.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
RenderViewPerspective::RenderViewPerspective(const RenderView::Config& config)
    : RenderView(config) {
    const auto pers_cfg = static_cast<const Config*>(&config);

    _fov         = pers_cfg->fov;
    _near_clip   = pers_cfg->near_clip;
    _far_clip    = pers_cfg->far_clip;
    _camera      = pers_cfg->camera;
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);
}
RenderViewPerspective::~RenderViewPerspective() {}

// ////////////////////////////////////// //
// RENDER VIEW PERSPECTIVE PUBLIC METHODS //
// ////////////////////////////////////// //

void RenderViewPerspective::on_resize(const uint32 width, const uint32 height) {
    RenderView::on_resize(width, height);
    _proj_matrix = glm::perspective(
        _fov, (float32) _width / _height, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);
}

Vector<GeometryRenderData>& RenderViewPerspective::get_all_render_data() {
    if (_all_render_data.size() == 0) {
        for (const auto& mesh : _potentially_visible_meshes) {
            const auto model_matrix = mesh->transform.world();
            for (const auto& geo : mesh->geometries()) {
                _all_render_data.push_back( //
                    { geo, geo->material, model_matrix }
                );
            }
        }
    }
    return _all_render_data;
}

Vector<GeometryRenderData>& RenderViewPerspective::get_visible_render_data(
    const uint32 frame_number
) {
    // Only update once per frame
    if (_last_frame == frame_number) return _visible_render_data;
    _last_frame = frame_number;

    // Clear geometry data
    _visible_render_data.clear();

    // Keep a list of transparent objects
    typedef std::pair<float32, GeometryRenderData> TGeomData;

    // TODO: Performance :/
    static Vector<TGeomData> transparent_geometries {};
    transparent_geometries.clear();

    // Create frustum for culling
    const auto forward = _camera->forward();
    const auto right   = -_camera->left();
    const auto up      = glm::cross(right, forward);
    Frustum    frustum {
        _camera->transform.position(), forward, right,      up,
        (float32) _width / _height,    _fov,    _near_clip, _far_clip
    };

    // Add all opaque geometries
    for (const auto& mesh : _potentially_visible_meshes) {
        const auto model_matrix = mesh->transform.world();
        for (auto* const geom : mesh->geometries()) {
            // Check if geometry is inside view frustum
            const auto geom_3d = static_cast<Geometry3D*>(geom);
            const auto aabb    = geom_3d->bbox.get_transformed(model_matrix);
            if (!frustum.contains(aabb))
                // We are skipping this geometry. It wont be rendered
                continue;

            // Create render data
            const GeometryRenderData render_data { geom,
                                                   geom->material,
                                                   model_matrix };

            // TODO: Add something in material to check for transparency.
            if (geom->material()->diffuse_map()->texture->has_transparency() ==
                false)
                _visible_render_data.push_back(render_data);
            else {
                const auto geom3d   = dynamic_cast<Geometry3D*>(geom);
                const auto center   = geom3d->bbox.get_center();
                const auto distance = glm::distance(
                    _camera->transform.position(),
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
        _visible_render_data.push_back(t_geom.second);

    return _visible_render_data;
}

} // namespace ENGINE_NAMESPACE