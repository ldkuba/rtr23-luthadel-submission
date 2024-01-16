#include "app/app_temp.hpp"

#include "resources/loaders/mesh_loader.hpp"
#include "timer.hpp"
#include <chrono>

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
TestApplication::TestApplication() {}
TestApplication::~TestApplication() { del(_app_surface); }

// /////////////////////// //
// APP TEMP PUBLIC METHODS //
// /////////////////////// //

void TestApplication::run() {
    // === Setup ===
    setup_camera();
    setup_input();
    setup_render_passes();
    _material_system.initialize(); // TODO: This is stupid imo :(
    setup_views();
    setup_modules();
    setup_scene_geometry(2);
    setup_lights();

    // === Performance meter ===
    Timer& timer = Timer::global_timer;

    // === Main loop ===
    while (!_app_surface->should_close() && _app_should_close == false) {
        const auto delta_time   = calculate_delta_time();
        const auto elapsed_time = calculate_elapsed_time();

        // FPS COUNTER
        static float64 last_print  = 0;
        static uint64  frame_count = 0;
        frame_count++;
        if (std::abs(last_print - elapsed_time) > 1.0 && _log_fps) {
            const auto sec = std::abs(last_print - elapsed_time);
            const auto fps = frame_count / sec;
            Logger::debug("FPS: ", fps);
            last_print  = elapsed_time;
            frame_count = 0;
            timer.start();
        }

        timer.reset();
        _app_surface->process_events(delta_time);

        // TODO: Temp code; update one and only object
        if (_cube_rotation) {
            float rotation_speed = 1.0f * delta_time;
            for (auto& mesh : _world_mesh_data.meshes) {
                mesh->transform.rotate_by(
                    glm::vec3(0.0f, 0.0f, 1.0f), rotation_speed
                );
            }
        }

        // Directional light rotation example
        // Get value osculating between -0.2f and 0.2f using
        // std::chrono::system_clock
        const auto oscillating_value = [](float32 frequency,
                                          float32 amplitude,
                                          float32 phase = 0) {
            const auto now = std::chrono::system_clock::now();
            const auto now_ms =
                std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            const auto epoch = now_ms.time_since_epoch();
            const auto value =
                std::sin(2 * M_PI * frequency * epoch.count() / 1000.0 + phase);
            return value * amplitude;
        };

        if (_move_directional_light_flag) {
            const_cast<DirectionalLight*>(_light_system.get_directional())
                ->data.direction.x = oscillating_value(0.1f, 0.2f);
            const_cast<DirectionalLight*>(_light_system.get_directional())
                ->data.direction.y =
                oscillating_value(0.1f, 0.05f, M_PI / 2.0f) + 0.1f;
        }

        timer.time("Events processed in ");

        // Construct render packet
        Renderer::Packet packet {};
        // Add module render data
        for (const auto module : _modules)
            packet.module_data.push_back(module->build_pocket());

        timer.time("Packets packed in ");

        auto result = _app_renderer.draw_frame(&packet, delta_time);
        if (result.has_error()) {
            // TODO: PROCESS ERROR
            Logger::error(result.error().what());
        }

        timer.time("Frame rendered in ");
        timer.stop();
    }
}

// //////////////////////// //
// APP TEMP PRIVATE METHODS //
// //////////////////////// //

float64 TestApplication::calculate_delta_time() {
    static auto start_time   = Platform::get_absolute_time();
    auto        current_time = Platform::get_absolute_time();
    auto        delta_time   = current_time - start_time;
    start_time               = current_time;
    return delta_time;
}

float64 TestApplication::calculate_elapsed_time() {
    static auto start_time   = Platform::get_absolute_time();
    auto        current_time = Platform::get_absolute_time();
    return current_time - start_time;
}

// TODO: Still temp
void TestApplication::setup_camera() {
    _main_camera                     = _camera_system.default_camera;
    _main_camera->transform.position = glm::vec3(2, 2, 2);
    _main_camera->add_pitch(35);
    _main_camera->add_yaw(-135);
}

#define HoldControl(name, key)                                                 \
    auto name = _input_system.create_control(#name, ControlType::Hold)         \
                    .expect("ERROR :: CONTROL CREATION FAILED.");              \
    name->map_key(KeyCode::key)
#define PressControl(name, key)                                                \
    auto name = _input_system.create_control(#name, ControlType::Press)        \
                    .expect("ERROR :: CONTROL CREATION FAILED.");              \
    name->map_key(KeyCode::key)
#define ReleaseControl(name, key)                                              \
    auto name = _input_system.create_control(#name, ControlType::Release)      \
                    .expect("ERROR :: CONTROL CREATION FAILED.");              \
    name->map_key(KeyCode::key)

void TestApplication::setup_input() {
    _input_system.register_input_source(_app_surface);

    // === Definitions ===
    // Application controls
    ReleaseControl(close_app_control, ESCAPE);
    // Camera controls
    HoldControl(camera_forward_c, W);
    HoldControl(camera_backwards_c, S);
    HoldControl(camera_left_c, A);
    HoldControl(camera_right_c, D);
    HoldControl(camera_up_c, E);
    HoldControl(camera_down_c, Q);
    HoldControl(camera_rotate_left_c, J);
    HoldControl(camera_rotate_right_c, L);
    HoldControl(camera_rotate_up_c, I);
    HoldControl(camera_rotate_down_c, K);
    ReleaseControl(reset_camera, R);
    ReleaseControl(camera_position, C);
    // Rendering
    PressControl(mode_0_c, NUM_0);
    PressControl(mode_1_c, NUM_1);
    PressControl(mode_2_c, NUM_2);
    PressControl(mode_3_c, NUM_3);
    PressControl(mode_4_c, NUM_4);
    PressControl(mode_5_c, NUM_5);
    PressControl(mode_6_c, NUM_6);
    // Other
    PressControl(spin_cube, SPACE);
    PressControl(shader_reload, Z); // TODO:
    PressControl(toggle_ssr, X);
    PressControl(show_fps, F);
    PressControl(move_directional_light, B);

    // === Events ===
    // Application controls
    close_app_control->event +=
        [&](float64, float64) { _app_should_close = true; };

    // Camera: info, TODO: TEMP
    static const float32 camera_speed   = 5.0f;
    static const float32 rotation_speed = 100.0f;

    // Camera: movement
    camera_forward_c->event += [&](float32 dt, float32) {
        _main_camera->move_forwards(camera_speed * dt);
    };
    camera_backwards_c->event += [&](float32 dt, float32) {
        _main_camera->move_backwards(camera_speed * dt);
    };
    camera_left_c->event += [&](float32 dt, float32) { //
        _main_camera->move_left(camera_speed * dt);
    };
    camera_right_c->event += [&](float32 dt, float32) {
        _main_camera->move_right(camera_speed * dt);
    };
    camera_up_c->event +=
        [&](float32 dt, float32) { _main_camera->move_up(camera_speed * dt); };
    camera_down_c->event += [&](float32 dt, float32) {
        _main_camera->move_down(camera_speed * dt);
    };

    // Camera: rotation
    camera_rotate_right_c->event += [&](float32 dt, float32) { //
        _main_camera->add_yaw(-rotation_speed * dt);
    };
    camera_rotate_left_c->event += [&](float32 dt, float32) { //
        _main_camera->add_yaw(rotation_speed * dt);
    };
    camera_rotate_up_c->event += [&](float32 dt, float32) {
        _main_camera->add_pitch(-rotation_speed * dt);
    };
    camera_rotate_down_c->event += [&](float32 dt, float32) {
        _main_camera->add_pitch(rotation_speed * dt);
    };

    // Camera: other
    reset_camera->event += [&](float32, float32) {
        _main_camera->reset();
        _main_camera->transform.position = glm::vec3(2, 2, 2);
        _main_camera->add_pitch(35);
        _main_camera->add_yaw(-135);
    };
    camera_position->event += [&](float32, float32) {
        Logger::debug(_main_camera->transform.position());
    };

    // Rendering
    auto& wm = _module.world;
    mode_0_c->event +=
        [&wm](float32, float32) { wm->set_mode(DebugViewMode::Default); };
    mode_1_c->event +=
        [&wm](float32, float32) { wm->set_mode(DebugViewMode::Lighting); };
    mode_2_c->event +=
        [&wm](float32, float32) { wm->set_mode(DebugViewMode::Normals); };
    mode_3_c->event +=
        [&wm](float32, float32) { wm->set_mode(DebugViewMode::SSAO); };
    mode_4_c->event +=
        [&wm](float32, float32) { wm->set_mode(DebugViewMode::DefNoSSAO); };

    // Other
    spin_cube->event +=
        [&](float32, float32) { _cube_rotation = !_cube_rotation; };
    // TODO: Hot reload-able shaders
    shader_reload->event += [&](float32, float32) {
        Logger::debug("Reloading shaders...");
        _shader_system.reload_shaders();
    };
    toggle_ssr->event += [&](float32, float32) { _module.ssr->toggle(); };
    show_fps->event += [&](float32, float32) { _log_fps = !_log_fps; };
    move_directional_light->event += [&](float32, float32) {
        _move_directional_light_flag = !_move_directional_light_flag;
    };
}

void TestApplication::setup_render_passes() {
    // Link other systems
    _app_renderer.link_with_systems(&_texture_system);

    // === Render passes ===
    // Get width & height
    const auto width       = _app_surface->get_width_in_pixels();
    const auto height      = _app_surface->get_height_in_pixels();
    const auto half_width  = width / 2;
    const auto half_height = height / 2;

    const auto shadowmap_directional_size = 4096;

    // Create render passes
    const auto g_renderpass = _app_renderer.create_render_pass({
        .name          = RenderPass::BuiltIn::GPrePass,
        .depth_testing = true,
    });
    const auto ao_renderpass =
        _app_renderer.create_render_pass({ RenderPass::BuiltIn::AOPass });
    const auto blur_renderpass =
        _app_renderer.create_render_pass({ RenderPass::BuiltIn::BlurPass });
    const auto skybox_renderpass = _app_renderer.create_render_pass(
        { .name = RenderPass::BuiltIn::SkyboxPass, .multisampling = true }
    );
    const auto world_renderpass =
        _app_renderer.create_render_pass({ .name =
                                               RenderPass::BuiltIn::WorldPass,
                                           .depth_testing = true,
                                           .multisampling = true });
    const auto ui_renderpass =
        _app_renderer.create_render_pass({ RenderPass::BuiltIn::UIPass });
    const auto shadowmap_directional_renderpass =
        _app_renderer.create_render_pass(
            { .name          = RenderPass::BuiltIn::ShadowmapDirectionalPass,
              .clear_color   = glm::vec4(1.0f),
              .depth_testing = true }
        );
    const auto shadowmap_sampling_renderpass = _app_renderer.create_render_pass(
        { .name          = RenderPass::BuiltIn::ShadowmapSamplingPass,
          .depth_testing = true }
    );
    const auto ssr_renderpass =
        _app_renderer.create_render_pass({ RenderPass::BuiltIn::SSRPass });
    const auto volumetrics_renderpass = _app_renderer.create_render_pass(
        { RenderPass::BuiltIn::VolumetricsPass }
    );
    const auto volumetrics_blur_renderpass = _app_renderer.create_render_pass(
        { RenderPass::BuiltIn::VolumetricsBlurPass }
    );
    const auto pp_effects_renderpass = _app_renderer.create_render_pass(
        { RenderPass::BuiltIn::PostProcessingPass }
    );

    // === Create render target textures ===
    // G buffer
    const auto g_pre_pass_texture =
        _texture_system.create({ .name          = UsedTextures::GPrePassTarget,
                                 .width         = width,
                                 .height        = height,
                                 .channel_count = 4, // Channel count
                                 .format        = Texture::Format::RGBA32Sfloat,
                                 .is_writable   = true,
                                 .is_render_target = true });
    const auto depth_texture =
        _texture_system.create({ .name             = UsedTextures::DepthTarget,
                                 .width            = width,
                                 .height           = height,
                                 .channel_count    = 4,
                                 .format           = Texture::Format::D32,
                                 .is_writable      = true,
                                 .is_render_target = true });

    // SSAO
    const auto ssao_texture =
        _texture_system.create({ .name          = UsedTextures::SSAOPassTarget,
                                 .width         = half_width,
                                 .height        = half_height,
                                 .channel_count = 1,
                                 .format        = Texture::Format::RGBA32Sfloat,
                                 .is_writable   = true,
                                 .is_render_target = true });
    const auto blured_ssao_texture =
        _texture_system.create({ .name   = UsedTextures::BluredSSAOPassTarget,
                                 .width  = half_width,
                                 .height = half_height,
                                 .channel_count    = 1,
                                 .is_writable      = true,
                                 .is_render_target = true });

    // Shadow maps
    const auto shadowmap_directional_depth_texture = _texture_system.create(
        { .name             = UsedTextures::DirectionalShadowMapDepthTarget,
          .width            = shadowmap_directional_size,
          .height           = shadowmap_directional_size,
          .channel_count    = 4,
          .format           = Texture::Format::D32,
          .is_writable      = true,
          .is_render_target = true }
    );
    const auto shadowmap_sampled_texture =
        _texture_system.create({ .name   = UsedTextures::ShadowmapSampledTarget,
                                 .width  = width,
                                 .height = height,
                                 .channel_count = 4,
                                 .format        = Texture::Format::RGBA32Sfloat,
                                 .is_writable   = true,
                                 .is_render_target = true });

    // Volumetrics
    const auto volumetrics_texture =
        _texture_system.create({ .name   = UsedTextures::VolumetricsTarget,
                                 .width  = half_width,
                                 .height = half_height,
                                 .channel_count    = 4,
                                 .is_writable      = true,
                                 .is_render_target = true });
    const auto volumetrics_blur_texture =
        _texture_system.create({ .name   = UsedTextures::VolumetricsBlurTarget,
                                 .width  = half_width,
                                 .height = half_height,
                                 .channel_count    = 4,
                                 .is_writable      = true,
                                 .is_render_target = true });

    // World pass color target
    const auto world_pass_texture =
        _texture_system.create({ .name          = UsedTextures::WorldPassTarget,
                                 .width         = width,
                                 .height        = height,
                                 .channel_count = 4,
                                 .is_writable   = true,
                                 .is_render_target = true });
    const auto world_pass_ms_texture =
        _texture_system.create({ .name   = UsedTextures::WorldPassMSTarget,
                                 .width  = width,
                                 .height = height,
                                 .channel_count   = 4,
                                 .is_writable     = true,
                                 .is_multisampled = true });

    // SSR target
    const auto ssr_texture =
        _texture_system.create({ .name             = UsedTextures::SSRTarget,
                                 .width            = width,
                                 .height           = height,
                                 .channel_count    = 4,
                                 .is_writable      = true,
                                 .is_render_target = true });

    // === Create render targets ===
    // G pre pass
    g_renderpass->add_render_target(
        { width, height, { g_pre_pass_texture, depth_texture }, true }
    );
    // AO
    ao_renderpass->add_render_target({ half_width,
                                       half_height,
                                       { ssao_texture },
                                       true,
                                       RenderTarget::SynchMode::HalfResolution }
    );
    blur_renderpass->add_render_target(
        { half_width,
          half_height,
          { blured_ssao_texture },
          true,
          RenderTarget::SynchMode::HalfResolution }
    );
    // Shadows
    shadowmap_directional_renderpass->add_render_target(
        { shadowmap_directional_size,
          shadowmap_directional_size,
          { shadowmap_directional_depth_texture },
          true,
          RenderTarget::SynchMode::None }
    );
    shadowmap_directional_renderpass->disable_color_output();
    shadowmap_sampling_renderpass->add_render_target(
        { width,
          height,
          { shadowmap_sampled_texture, _app_renderer.get_depth_texture() },
          true }
    );
    // Volumetrics
    volumetrics_renderpass->add_render_target(
        { half_width,
          half_height,
          { volumetrics_texture },
          true,
          RenderTarget::SynchMode::HalfResolution }
    );
    volumetrics_blur_renderpass->add_render_target(
        { half_width,
          half_height,
          { volumetrics_blur_texture },
          true,
          RenderTarget::SynchMode::HalfResolution }
    );
    // Main
    skybox_renderpass->add_render_target(
        { width, height, { world_pass_ms_texture, world_pass_texture }, true }
    );
    world_renderpass->add_render_target(
        { width,
          height,
          { world_pass_ms_texture,
            _app_renderer.get_ms_depth_texture(),
            world_pass_texture },
          true }
    );
    // Post processing
    ssr_renderpass->add_render_target({ width, height, { ssr_texture }, true });
    pp_effects_renderpass->add_window_as_render_target();
    // UI
    ui_renderpass->add_window_as_render_target();

    // === Initialize ===
    RenderPass::start >>
        // Depth normals
        "CDS" >> g_renderpass >>
        // SSAO
        "C" >> ao_renderpass >> "C" >> blur_renderpass >>
        // Directional Shadow-mapping
        "DS" >> shadowmap_directional_renderpass >> "CDS" >>
        shadowmap_sampling_renderpass >>
        // Volumetrics
        "CDS" >> volumetrics_renderpass >> "C" >> volumetrics_blur_renderpass >>
        // Skybox
        "C" >> skybox_renderpass >>
        // World
        "DS" >> world_renderpass >>
        // Post process
        "C" >> ssr_renderpass >> "C" >> pp_effects_renderpass >>
        // UI
        ui_renderpass >>
        // Finish
        RenderPass::finish;
}

void TestApplication::setup_views() {
    const auto width  = _app_surface->get_width_in_pixels();
    const auto height = _app_surface->get_height_in_pixels();

    // Config
    RenderViewPerspective::Config main_world_view_config {
        "MainWorldView",    width, height, RenderView::Type::DefaultPerspective,
        glm::radians(45.0), 0.1,   1000.0, _main_camera
    };
    RenderViewOrthographic::Config main_ui_view_config {
        "MainUIView", width, height,      RenderView::Type::DefaultOrthographic,
        -100.0,       100.0, _main_camera
    };
    RenderViewOrthographic::Config dir_light_view_config {
        "DirLightView", width,
        height,         RenderView::Type::DefaultOrthographic,
        -100.0,         100.0,
        _main_camera
    };

    // Create views
    _main_world_view = static_cast<RenderViewPerspective*>(
        _render_view_system.create(main_world_view_config)
            .expect("Render View creation failed.")
    );
    _main_ui_view = static_cast<RenderViewOrthographic*>(
        _render_view_system.create(main_ui_view_config)
            .expect("Render View creation failed.")
    );
    _dir_light_view = static_cast<RenderViewOrthographic*>(
        _render_view_system.create(dir_light_view_config)
            .expect("Render View creation failed.")
    );
}

void TestApplication::setup_modules() {
    // Create render modules
    _module.g_pass = _render_module_system.create<RenderModuleGPrepass>(
        { Shader::BuiltIn::GPrePassShader,
          RenderPass::BuiltIn::GPrePass,
          _main_world_view }
    );
    _module.ao = _render_module_system.create<RenderModuleAO>(
        { Shader::BuiltIn::AOShader,
          RenderPass::BuiltIn::AOPass,
          _main_world_view,
          UsedTextures::GPrePassTarget,
          UsedTextures::DepthTarget }
    );
    _module.blur = _render_module_system.create<RenderModulePostProcessing>(
        { Shader::BuiltIn::BlurShader,
          RenderPass::BuiltIn::BlurPass,
          _main_world_view,
          UsedTextures::SSAOPassTarget }
    );
    _module.shadow_dir =
        _render_module_system.create<RenderModuleShadowmapDirectional>(
            { Shader::BuiltIn::ShadowmapDirectionalShader,
              RenderPass::BuiltIn::ShadowmapDirectionalPass,
              _dir_light_view }
        );
    _module.shadow_sampling =
        _render_module_system.create<RenderModuleShadowmapSampling>(
            { Shader::BuiltIn::ShadowmapSamplingShader,
              RenderPass::BuiltIn::ShadowmapSamplingPass,
              _main_world_view,
              UsedTextures::DepthTarget,
              UsedTextures::DirectionalShadowMapDepthTarget }
        );
    _module.volumetrics = _render_module_system.create<RenderModuleVolumetrics>(
        { Shader::BuiltIn::VolumetricsShader,
          RenderPass::BuiltIn::VolumetricsPass,
          _main_world_view,
          UsedTextures::DepthTarget,
          UsedTextures::DirectionalShadowMapDepthTarget }
    );
    _module.volumetrics_blur =
        _render_module_system.create<RenderModulePostProcessing>(
            { Shader::BuiltIn::VolumetricsBlurShader,
              RenderPass::BuiltIn::VolumetricsBlurPass,
              _main_world_view,
              UsedTextures::VolumetricsTarget }
        );
    _module.skybox = _render_module_system.create<RenderModuleSkybox>(
        { Shader::BuiltIn::SkyboxShader,
          RenderPass::BuiltIn::SkyboxPass,
          _main_world_view,
          "skybox/skybox" }
    );
    _module.world = _render_module_system.create<RenderModuleWorld>(
        { Shader::BuiltIn::MaterialShader,
          RenderPass::BuiltIn::WorldPass,
          _main_world_view,
          UsedTextures::BluredSSAOPassTarget,
          UsedTextures::ShadowmapSampledTarget,
          UsedTextures::VolumetricsBlurTarget,
          glm::vec4(0.05f, 0.05f, 0.05f, 1.0f) }
    );
    _module.ui = _render_module_system.create<RenderModuleUI>(
        { Shader::BuiltIn::UIShader,
          RenderPass::BuiltIn::UIPass,
          _main_ui_view }
    );
    _module.ssr = _render_module_system.create<RenderModuleSSR>(
        { Shader::BuiltIn::SSRShader,
          RenderPass::BuiltIn::SSRPass,
          _main_world_view,
          UsedTextures::WorldPassTarget,
          UsedTextures::GPrePassTarget,
          UsedTextures::DepthTarget }
    );
    _module.pp_effects =
        _render_module_system.create<RenderModulePostProcessingEffects>(
            { Shader::BuiltIn::PostProcessingEffShader,
              RenderPass::BuiltIn::PostProcessingPass,
              _main_world_view,
              UsedTextures::SSRTarget,
              UsedTextures::DepthTarget }
        );

    // Fill list of render commands
    _modules.push_back(_module.g_pass);
    _modules.push_back(_module.ao);
    _modules.push_back(_module.blur);
    _modules.push_back(_module.shadow_dir);
    _modules.push_back(_module.shadow_sampling);
    _modules.push_back(_module.volumetrics);
    _modules.push_back(_module.volumetrics_blur);
    _modules.push_back(_module.skybox);
    _modules.push_back(_module.world);
    _modules.push_back(_module.ssr);
    _modules.push_back(_module.pp_effects);
    _modules.push_back(_module.ui);
}

void TestApplication::setup_scene_geometry(const uint32 scene_id) {
    Vector<Mesh*> meshes {};
    Vector<Mesh*> ui_meshes {};

    if (scene_id == 0) {
        // Create geometries
        Geometry* const geometry_1 =
            _geometry_system.generate_cube("cube", "test_material");
        Geometry* const geometry_2 =
            _geometry_system.generate_cube("cube", "test_material");
        Geometry* const geometry_3 =
            _geometry_system.generate_cube("cube", "test_material");

        // Create meshes
        Mesh* const mesh_1 = new (MemoryTag::Geometry) Mesh(geometry_1);
        Mesh* const mesh_2 = new (MemoryTag::Geometry) Mesh(geometry_2);
        Mesh* const mesh_3 = new (MemoryTag::Geometry) Mesh(geometry_3);

        // Mesh 2 transform
        mesh_2->transform.scale_by(0.4f);
        mesh_2->transform.rotate_by_deg(glm::vec3(0, 1, 0), 30.f);
        mesh_2->transform.translate_by(glm::vec3(0.f, 1.f, 0.f));
        mesh_2->transform.parent = &mesh_1->transform;

        // Mesh 3 transform
        mesh_3->transform.scale_by(0.2f);
        mesh_3->transform.rotate_by_deg(glm::vec3(1, 0, 0), -30.f);
        mesh_3->transform.translate_by(glm::vec3(0.f, 1.f, 0.f));
        mesh_3->transform.parent = &mesh_2->transform;

        // Add meshes
        meshes.push_back(mesh_1);
        meshes.push_back(mesh_2);
        meshes.push_back(mesh_3);
    } else {
        const auto obj_name = (scene_id == 1)   ? "falcon"
                              : (scene_id == 2) ? "sponza"
                                                : "viking_room";

        /// Load MESH TEST
        MeshLoader loader {};
        const auto load_result = loader.load(obj_name);
        if (load_result.has_error()) {
            Logger::error(load_result.error().what());
            Logger::fatal("Mesh loading failed");
        }
        const auto config_array =
            dynamic_cast<GeometryConfigArray*>(load_result.value());

        // Add all geometries
        Vector<Geometry*> geometries {};
        geometries.reserve(config_array->configs.size());
        for (auto config : config_array->configs)
            geometries.push_back(_geometry_system.acquire(*config));

        // Add mesh
        Mesh* const mesh = new (MemoryTag::Geometry) Mesh(geometries);
        meshes.push_back(mesh);

        // Transform mesh as needed
        switch (scene_id) {
        case 2:
            mesh->transform.scale_by(0.02f);
            mesh->transform.translate_by(glm::vec3(0, -1, 0));
        case 1: //
            mesh->transform.rotate_by_deg(glm::vec3(1, 0, 0), 90.f);
            break;
        }
    }

    /// Load GUI TEST
    // Create geometry
    const auto geom_2d = _geometry_system.generate_ui_rectangle(
        "ui", 128, 128, "test_ui_material"
    );

    // === Mesh render data ===
    // Create mesh
    Mesh* mesh_ui = new (MemoryTag::Geometry) Mesh(geom_2d);
    ui_meshes.push_back(mesh_ui);

    // Create mesh data
    _world_mesh_data = { meshes };
    _ui_mesh_data    = { ui_meshes };

    // Set mesh data
    _main_world_view->set_visible_meshes(_world_mesh_data.meshes);
    _main_ui_view->set_visible_meshes(_ui_mesh_data.meshes);
    _dir_light_view->set_visible_meshes(_world_mesh_data.meshes);

    // TODO: TEMP
    _module.g_pass->initialize_shader_data();
}

void TestApplication::setup_lights() {
    const auto directional_light = new (MemoryTag::Scene)
        DirectionalLight { "dir_light",
                           { glm::vec4(0.0f, 0.2f, -1.0, 1.0),
                             glm::vec4(0.5, 0.5, 0.5, 1.0) } };
    _light_system.add_directional(directional_light);

    const auto pl0 =
        new (MemoryTag::Scene) PointLight { "pl0",
                                            { glm::vec4(1.0, 1.0, 2.0, 1.0),
                                              glm::vec4(0.0, 5.0, 0.0, 1.0),
                                              1.0,
                                              0.35,
                                              0.44,
                                              0.0 } };
    const auto pl1 =
        new (MemoryTag::Scene) PointLight { "pl1",
                                            { glm::vec4(5.0, 1.0, 2.0, 1.0),
                                              glm::vec4(5.0, 0.0, 0.0, 1.0),
                                              1.0,
                                              0.35,
                                              0.44,
                                              0.0 } };
    _light_system.add_point(pl0);
    _light_system.add_point(pl1);
}

} // namespace ENGINE_NAMESPACE