#pragma once

#include "math_libs.hpp"
#include "logger.hpp"

#include "systems/geometry_system.hpp"
#include "systems/render_view_system.hpp"
#include "systems/input/input_system.hpp"
#include "resources/mesh.hpp"
#include "renderer/views/render_view_world.hpp"
#include "renderer/views/render_view_ui.hpp"

namespace ENGINE_NAMESPACE {

class TestApplication {
  public:
    TestApplication();
    ~TestApplication();

    void run();

  private:
    // Surface
    Platform::Surface* _app_surface =
        Platform::Surface::get_instance(800, 600, std::string(APP_NAME));

    // Renderer
    Renderer _app_renderer { RendererBackendType::Vulkan, _app_surface };

    // Main camera
    Camera* main_camera {};

    // Systems
    InputSystem      _input_system {};
    CameraSystem     _camera_system {};
    ResourceSystem   _resource_system {};
    TextureSystem    _texture_system { &_app_renderer, &_resource_system };
    ShaderSystem     _shader_system { &_app_renderer,
                                  &_resource_system,
                                  &_texture_system };
    RenderViewSystem _render_view_system { &_shader_system,
                                           &_camera_system,
                                           _app_surface };
    MaterialSystem   _material_system {
        &_app_renderer, &_resource_system, &_texture_system, &_shader_system
    };
    GeometrySystem _geometry_system { &_app_renderer, &_material_system };

    float64 calculate_delta_time();

    // TODO: TEMP
    bool _app_should_close = false;
    bool _cube_rotation    = false;

    RenderViewWorld* _ow_render_view;
    RenderViewUI*    _ui_render_view;

    void setup_input();
};

} // namespace ENGINE_NAMESPACE