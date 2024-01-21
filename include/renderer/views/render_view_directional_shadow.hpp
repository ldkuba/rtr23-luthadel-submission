#pragma once
#include "renderer/camera.hpp"
#include "renderer/views/render_view_orthographic.hpp"

namespace ENGINE_NAMESPACE {

class RenderViewDirectionalShadow : public RenderViewOrthographic {
  public:
    struct Config : public RenderView::Config {
        float32 near_clip;
        float32 far_clip;
        Camera* camera;
    };
  public:
    RenderViewDirectionalShadow(const RenderView::Config& config);
    ~RenderViewDirectionalShadow();

    virtual void on_resize(const uint32 width, const uint32 height) override;

    glm::mat4 get_view_matrix(glm::vec3 light_dir) const;
    glm::vec3 get_light_camera_position(glm::vec3 light_dir) const;

    Vector<GeometryRenderData>& get_visible_render_data(
        const uint32 frame_number
    ) override;
};

} // namespace ENGINE_NAMESPACE
