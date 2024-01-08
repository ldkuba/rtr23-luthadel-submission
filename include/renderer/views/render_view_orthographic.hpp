#pragma once

#include "render_view.hpp"
#include "renderer/camera.hpp"

namespace ENGINE_NAMESPACE {

class RenderViewOrthographic : public RenderView {
  public:
    struct Config : public RenderView::Config {
        float32 near_clip;
        float32 far_clip;
        Camera* camera;
    };

  public:
    Property<Camera*> camera {
        GET { return _camera; }
    };

    RenderViewOrthographic(const RenderView::Config& config);
    ~RenderViewOrthographic();

    virtual void on_resize(const uint32 width, const uint32 height) override;

    Vector<GeometryRenderData>& get_visible_render_data(
        const uint32 frame_number
    ) override;

  protected:
    float32 _near_clip;
    float32 _far_clip;
    Camera* _camera;
};

} // namespace ENGINE_NAMESPACE