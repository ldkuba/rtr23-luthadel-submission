#pragma once

#include "renderer/views/render_view.hpp"
#include "systems/camera_system.hpp"

namespace ENGINE_NAMESPACE {

class ShaderSystem;

class RenderViewSystem {
  public:
    RenderViewSystem(
        ShaderSystem* const      shader_system,
        CameraSystem* const      camera_system,
        Platform::Surface* const surface
    );
    ~RenderViewSystem();

    Result<RenderView*, RuntimeError> create(const RenderView::Config& config);
    Result<RenderView*, RuntimeError> acquire(const String& name);

  private:
    ShaderSystem* const _shader_system;
    CameraSystem* const _camera_system;

    UnorderedMap<String, RenderView*> _registered_views;

    void on_window_resize(const uint32 width, const uint32 height);
    Result<void, RuntimeError> name_is_valid(const String& name);
};

} // namespace ENGINE_NAMESPACE