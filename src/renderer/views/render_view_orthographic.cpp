#include "renderer/views/render_view_orthographic.hpp"

#include "resources/mesh.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
RenderViewOrthographic::RenderViewOrthographic(const RenderView::Config& config)
    : RenderView(config) {
    const auto pers_cfg = static_cast<const Config*>(&config);

    _near_clip   = pers_cfg->near_clip;
    _far_clip    = pers_cfg->far_clip;
    _camera      = pers_cfg->camera;
    _proj_matrix = glm::ortho(
        0.0f, (float32) _width, (float32) _height, 0.f, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);
}
RenderViewOrthographic::~RenderViewOrthographic() {}

// ////////////////////////////////////// //
// RENDER VIEW PERSPECTIVE PUBLIC METHODS //
// ////////////////////////////////////// //

void RenderViewOrthographic::on_resize(
    const uint32 width, const uint32 height
) {
    RenderView::on_resize(width, height);
    _proj_matrix = glm::ortho(
        0.0f, (float32) _width, (float32) _height, 0.f, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);
}

Vector<GeometryRenderData>& RenderViewOrthographic::get_visible_render_data(
    const uint32 frame_number
) {
    // Only update once per frame
    if (_last_frame == frame_number) return _visible_render_data;
    _last_frame = frame_number;

    // Clear geometry data
    _visible_render_data.clear();

    // Update render data
    for (const auto& mesh : _potentially_visible_meshes) {
        const auto model_matrix = mesh->transform.world();
        for (auto* const geom : mesh->geometries())
            _visible_render_data.push_back(
                { geom, geom->material, model_matrix }
            );
    }

    return _visible_render_data;
}

} // namespace ENGINE_NAMESPACE