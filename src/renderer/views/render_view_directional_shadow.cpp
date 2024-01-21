#include "renderer/views/render_view_directional_shadow.hpp"
#include "resources/mesh.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
RenderViewDirectionalShadow::RenderViewDirectionalShadow(const RenderView::Config& config)
    : RenderViewOrthographic(config) {

    _proj_matrix = glm::ortho(
        -1.0f * (float32) _width, (float32) _width, -1.0f * (float32) _height, (float32) _height, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);
}
RenderViewDirectionalShadow::~RenderViewDirectionalShadow() {}

// ////////////////////////////////////// //
// RENDER VIEW PERSPECTIVE PUBLIC METHODS //
// ////////////////////////////////////// //

void RenderViewDirectionalShadow::on_resize(
    const uint32 width, const uint32 height
) {
    RenderView::on_resize(width, height);
    _proj_matrix = glm::ortho(
        -1.0f * (float32) _width, (float32) _width, -1.0f * (float32) _height, (float32) _height, _near_clip, _far_clip
    );
    _proj_inv_matrix = glm::inverse(_proj_matrix);
}

glm::mat4 RenderViewDirectionalShadow::get_view_matrix(glm::vec3 light_dir) const {
    glm::vec3 up_vector;
    if (glm::abs(
            glm::dot(light_dir, glm::vec3(0.0f, 1.0f, 0.0f))
        ) > 0.99f) {
        up_vector = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        up_vector = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    auto camera_pos = _camera->transform.position();

    glm::vec3 light_pos = get_light_camera_position(light_dir);
    return glm::lookAt(
        light_pos,
        camera_pos,
        up_vector
    );
}

glm::vec3 RenderViewDirectionalShadow::get_light_camera_position(glm::vec3 light_dir) const {
    return _camera->transform.position() - glm::normalize(light_dir) * (_far_clip * 0.5f);
}

Vector<GeometryRenderData>& RenderViewDirectionalShadow::get_visible_render_data(
    const uint32 frame_number
) {
    // Only update once per frame
    if (_last_frame == frame_number) return _visible_render_data;
    _last_frame = frame_number;

    // Clear geometry data
    _visible_render_data.clear();

    // Update render data
    // TODO: add frustrum culling
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
