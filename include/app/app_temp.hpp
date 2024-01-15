#pragma once

#include "math_libs.hpp"
#include "logger.hpp"

#include "systems/camera_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/render_module_system.hpp"
#include "systems/render_view_system.hpp"
#include "systems/input/input_system.hpp"
#include "systems/light_system.hpp"
#include "resources/mesh.hpp"

#include "renderer/modules/render_module_world.hpp"
#include "renderer/modules/render_module_ui.hpp"
#include "renderer/modules/render_module_skybox.hpp"
#include "renderer/modules/render_module_g_prepass.hpp"
#include "renderer/modules/render_module_ao.hpp"
#include "renderer/modules/render_module_post_processing.hpp"
#include "renderer/modules/render_module_shadowmap_directional.hpp"
#include "renderer/modules/render_module_shadowmap_sampling.hpp"
#include "renderer/modules/render_module_volumetrics.hpp"
#include "renderer/modules/render_module_ssr.hpp"
#include "renderer/modules/render_module_post_processing_effects.hpp"

namespace ENGINE_NAMESPACE {

class TestApplication {
  public:
    TestApplication();
    ~TestApplication();

    void run();

  private:
    // Surface
    Platform::Surface* _app_surface =
        Platform::Surface::get_instance(1920, 1080, std::string(APP_NAME));

    // Renderer
    Renderer _app_renderer { RendererBackend::Type::Vulkan, _app_surface };

    // Main camera
    Camera* _main_camera {};

    // Systems
    InputSystem      _input_system {};
    CameraSystem     _camera_system {};
    ResourceSystem   _resource_system {};
    RenderViewSystem _render_view_system { &_app_renderer, _app_surface };
    TextureSystem    _texture_system { &_app_renderer, &_resource_system };
    ShaderSystem     _shader_system { &_app_renderer,
                                  &_resource_system,
                                  &_texture_system };
    MaterialSystem   _material_system {
        &_app_renderer, &_resource_system, &_texture_system, &_shader_system
    };
    GeometrySystem _geometry_system { &_app_renderer, &_material_system };
    LightSystem    _light_system { 10 };

    RenderModuleSystem _render_module_system { &_app_renderer,
                                               &_shader_system,
                                               &_texture_system,
                                               &_geometry_system,
                                               &_light_system };

    float64 calculate_delta_time();
    float64 calculate_elapsed_time();

    // TODO: TEMP
    bool _app_should_close            = false;
    bool _cube_rotation               = false;
    bool _log_fps                     = false;
    bool _move_directional_light_flag = false;

    // Render data
    MeshRenderData _world_mesh_data;
    MeshRenderData _ui_mesh_data;

    // Modules
    struct UsedModules {
        RenderModuleGPrepass*              g_pass;
        RenderModuleAO*                    ao;
        RenderModulePostProcessing*        blur;
        RenderModuleShadowmapDirectional*  shadow_dir;
        RenderModuleShadowmapSampling*     shadow_sampling;
        RenderModuleSkybox*                skybox;
        RenderModuleWorld*                 world;
        RenderModuleVolumetrics*           volumetrics;
        RenderModulePostProcessing*        volumetrics_blur;
        RenderModuleSSR*                   ssr;
        RenderModulePostProcessingEffects* pp_effects;
        RenderModuleUI*                    ui;
    };
    UsedModules _module {};

    Vector<RenderModule*> _modules {};

    // Textures
    struct UsedTextures {
        STRING_ENUM(GPrePassTarget);
        STRING_ENUM(LowResDepthTarget);
        STRING_ENUM(SSAOPassTarget);
        STRING_ENUM(BluredSSAOPassTarget);
        STRING_ENUM(DirectionalShadowMapDepthTarget);
        STRING_ENUM(ShadowmapSampledTarget);
        STRING_ENUM(VolumetricsTarget);
        STRING_ENUM(VolumetricsBlurTarget);
        STRING_ENUM(WorldPassTarget);
        STRING_ENUM(WorldPassMSTarget);
        STRING_ENUM(SSRTarget);
    };

    // Views
    RenderViewPerspective*  _main_world_view;
    RenderViewOrthographic* _main_ui_view;
    RenderViewOrthographic* _dir_light_view;

    void setup_camera();
    void setup_input();
    void setup_render_passes();
    void setup_views();
    void setup_modules();
    void setup_scene_geometry(const uint32 scene_id);
    void setup_lights();
};

} // namespace ENGINE_NAMESPACE