#include "app/app_temp.hpp"

#include "resources/loaders/mesh_loader.hpp"
#include "timer.hpp"
#include <chrono>

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
TestApplication::TestApplication() {}
TestApplication::~TestApplication() {
    // Temp
    _app_renderer.skybox_shader->release_instance_resources(
        _default_skybox.instance_id
    );
    _texture_system.release(_default_skybox.cube_map()->texture->name());
    _app_renderer.destroy_texture_map(_default_skybox.cube_map);
    _app_renderer.destroy_texture_map(_ssao_map);
    _app_renderer.destroy_texture_map(_shadowmap_directional_map);

    del(_app_surface);
}

// /////////////////////// //
// APP TEMP PUBLIC METHODS //
// /////////////////////// //

void TestApplication::run() {
    // === Setup ===
    setup_camera();
    setup_input();
    setup_render_passes();
    setup_scene_geometry();
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
        // Get value oscilating between -0.2f and 0.2f using std::chrono::system_clock
        const auto oscillating_value = [](float32 frequency, float32 amplitude, float32 phase = 0) {
            const auto now = std::chrono::system_clock::now();
            const auto now_ms =
                std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            const auto epoch = now_ms.time_since_epoch();
            const auto value =
                std::sin(2 * M_PI * frequency * epoch.count() / 1000.0 + phase);
            return value * amplitude;
        };

        if(_move_directional_light_flag) {
            const_cast<DirectionalLight*>(_light_system.get_directional())->data.direction.x = oscillating_value(0.1f, 0.2f);
            const_cast<DirectionalLight*>(_light_system.get_directional())->data.direction.y = oscillating_value(0.1f, 0.05f, M_PI / 2.0f) + 0.1f;
        }

        timer.time("Events processed in ");

        // Construct render packet
        Renderer::Packet packet {};
        // Add views
        packet.view_data.push_back(_de_render_view->on_build_pocket());
        packet.view_data.push_back(_ao_render_view->on_build_pocket());
        packet.view_data.push_back(_bl_render_view->on_build_pocket());
        packet.view_data.push_back(_smd_render_view->on_build_pocket());
        packet.view_data.push_back(_sb_render_view->on_build_pocket());
        packet.view_data.push_back(_ow_render_view->on_build_pocket());
        packet.view_data.push_back(_ui_render_view->on_build_pocket());

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
    PressControl(shader_reload, Z);
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
    auto& rv = _ow_render_view;
    mode_0_c->event +=
        [&rv](float32, float32) { rv->render_mode = DebugViewMode::Default; };
    mode_1_c->event +=
        [&rv](float32, float32) { rv->render_mode = DebugViewMode::Lighting; };
    mode_2_c->event +=
        [&rv](float32, float32) { rv->render_mode = DebugViewMode::Normals; };
    mode_3_c->event +=
        [&rv](float32, float32) { rv->render_mode = DebugViewMode::SSAO; };
    mode_4_c->event +=
        [&rv](float32, float32) { rv->render_mode = DebugViewMode::DefNoSSAO; };

    // Other
    spin_cube->event +=
        [&](float32, float32) { _cube_rotation = !_cube_rotation; };
    shader_reload->event +=
        [&](float32, float32) { _app_renderer.material_shader->reload(); };
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
    const auto width  = _app_surface->get_width_in_pixels();
    const auto height = _app_surface->get_height_in_pixels();

    const auto shadowmap_directional_size = 4096;

    // Create render passes
    const auto world_renderpass =
        _app_renderer.get_renderpass(RenderPass::BuiltIn::WorldPass)
            .expect("World pass somehow absent.");
    const auto depth_renderpass  = _app_renderer.create_render_pass({
        RenderPass::BuiltIn::DepthPass,       // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        true,                                 // Depth testing
        false                                 // Multisampling
    });
    const auto ao_renderpass     = _app_renderer.create_render_pass({
        RenderPass::BuiltIn::AOPass,          // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        false,                                // Depth testing
        false                                 // Multisampling
    });
    const auto blur_renderpass   = _app_renderer.create_render_pass({
        RenderPass::BuiltIn::BlurPass,        // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        false,                                // Depth testing
        false                                 // Multisampling
    });
    const auto skybox_renderpass = _app_renderer.create_render_pass({
        RenderPass::BuiltIn::SkyboxPass,      // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        false,                                // Depth testing
        true                                  // Multisampling
    });
    const auto ui_renderpass     = _app_renderer.create_render_pass({
        RenderPass::BuiltIn::UIPass,          // Name
        glm::vec2 { 0, 0 },                   // Draw offset
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f }, // Clear color
        false,                                // Depth testing
        false                                 // Multisampling
    });
    const auto shadowmap_directional_renderpass =
        _app_renderer.create_render_pass({
            RenderPass::BuiltIn::ShadowmapDirectionalPass, // Name
            glm::vec2 { 0, 0 },                            // Draw offset
            glm::vec4 { 1.0f, 1.0f, 1.0f, 1.0f },          // Clear color
            true,                                          // Depth testing
            false                                          // Multisampling
        });

    // Create render target textures
    const auto depth_normals_texture = _texture_system.acquire_writable(
        "DepthPrePassTarget",
        width,
        height,
        4,
        Texture::Format::RGBA32Sfloat,
        true
    );

    const auto ssao_texture = _texture_system.acquire_writable( //
        "SSAOPassTarget",
        width,
        height,
        1,
        Texture::Format::RGBA8Unorm,
        true
    );

    const auto blured_ssao_texture = _texture_system.acquire_writable(
        "BluredSSAOPassTarget",
        width,
        height,
        1,
        Texture::Format::RGBA8Unorm,
        true
    );

    // Shadowmap Directional Textures
    const auto shadowmap_directional_color_texture =
        _texture_system.acquire_writable(
            "DirectionalShadowMapColorTarget",
            shadowmap_directional_size,
            shadowmap_directional_size,
            4,
            Texture::Format::RGBA32Sfloat,
            true
        );

    const auto shadowmap_directional_depth_texture =
        _texture_system.acquire_writable(
            "DirectionalShadowMapDepthTarget",
            shadowmap_directional_size,
            shadowmap_directional_size,
            4,
            Texture::Format::D32,
            true
        );

    // Create render targets
    ui_renderpass->add_window_as_render_target();
    skybox_renderpass->add_window_as_render_target();
    depth_renderpass->add_render_target({ width,
                                          height,
                                          { depth_normals_texture,
                                            _app_renderer.get_depth_texture() },
                                          true });
    ao_renderpass->add_render_target({ width, height, { ssao_texture }, true });
    blur_renderpass->add_render_target(
        { width, height, { blured_ssao_texture }, true }
    );
    shadowmap_directional_renderpass->add_render_target(
        { shadowmap_directional_size,
          shadowmap_directional_size,
          { shadowmap_directional_color_texture,
            shadowmap_directional_depth_texture },
          true,
          false }
    );

    // shadowmap_directional_renderpass->disable_color_output();

    // Initialize AO only
    RenderPass::start >>
        // SSAO
        "CDS" >> depth_renderpass >> "C" >> ao_renderpass >> "C" >>
        blur_renderpass >>
        // Directional Shadowmapping
        "DS" >> shadowmap_directional_renderpass >>
        // Main render loop
        "C" >> skybox_renderpass >> "DS" >> world_renderpass >> ui_renderpass >>
        RenderPass::finish;

    // === Shaders ===
    // Create shaders
    _app_renderer.material_shader =
        _shader_system.acquire(Shader::BuiltIn::MaterialShader)
            .expect("Failed to load builtin material shader.");
    _app_renderer.ui_shader = //
        _shader_system.acquire(Shader::BuiltIn::UIShader)
            .expect("Failed to load builtin ui shader.");
    _app_renderer.skybox_shader =
        _shader_system.acquire(Shader::BuiltIn::SkyboxShader)
            .expect("Failed to load builtin skybox shader.");
    _app_renderer.depth_shader =
        _shader_system.acquire(Shader::BuiltIn::DepthShader)
            .expect("Failed to load builtin depth shader.");
    _app_renderer.ao_shader = //
        _shader_system.acquire(Shader::BuiltIn::AOShader)
            .expect("Failed to load builtin ao shader.");
    _app_renderer.ao_shader = //
        _shader_system.acquire(Shader::BuiltIn::BlurShader)
            .expect("Failed to load builtin blur shader.");
    _app_renderer.shadowmap_directional_shader = //
        _shader_system.acquire(Shader::BuiltIn::ShadowmapDirectionalShader)
            .expect("Failed to load builtin shadowmap directional shader.");

    _app_renderer.material_shader->reload();

    // === Render views ===
    // Configure
    RenderView::Config skybox_view_config {
        "skybox",
        Shader::BuiltIn::SkyboxShader,
        width,
        height,
        RenderView::Type::Skybox,
        RenderView::ViewMatrixSource::SceneCamera,
        RenderView::ProjectionMatrixSource::DefaultPerspective,
        { _app_renderer.get_renderpass(RenderPass::BuiltIn::SkyboxPass)
              .expect("Renderpass not found.") }
    };
    RenderView::Config opaque_world_view_config {
        "world_opaque",
        Shader::BuiltIn::MaterialShader,
        width,
        height,
        RenderView::Type::World,
        RenderView::ViewMatrixSource::SceneCamera,
        RenderView::ProjectionMatrixSource::DefaultPerspective,
        { _app_renderer.get_renderpass(RenderPass::BuiltIn::WorldPass)
              .expect("Renderpass not found.") }
    };
    RenderView::Config ui_view_config {
        "ui",
        Shader::BuiltIn::UIShader,
        width,
        height,
        RenderView::Type::UI,
        RenderView::ViewMatrixSource::SceneCamera,
        RenderView::ProjectionMatrixSource::DefaultPerspective,
        { _app_renderer.get_renderpass(RenderPass::BuiltIn::UIPass)
              .expect("Renderpass not found.") }
    };
    RenderView::Config depth_view_config {
        "depth",
        Shader::BuiltIn::DepthShader,
        width,
        height,
        RenderView::Type::Depth,
        RenderView::ViewMatrixSource::SceneCamera,
        RenderView::ProjectionMatrixSource::DefaultPerspective,
        { _app_renderer.get_renderpass(RenderPass::BuiltIn::DepthPass)
              .expect("Renderpass not found.") }
    };
    RenderView::Config ao_view_config {
        "ao",
        Shader::BuiltIn::AOShader,
        width,
        height,
        RenderView::Type::AO,
        RenderView::ViewMatrixSource::SceneCamera,
        RenderView::ProjectionMatrixSource::DefaultPerspective,
        { _app_renderer.get_renderpass(RenderPass::BuiltIn::AOPass)
              .expect("Renderpass not found.") }
    };
    RenderView::Config blur_view_config {
        "blur",
        Shader::BuiltIn::BlurShader,
        width,
        height,
        RenderView::Type::Blur,
        RenderView::ViewMatrixSource::SceneCamera,
        RenderView::ProjectionMatrixSource::DefaultPerspective,
        { _app_renderer.get_renderpass(RenderPass::BuiltIn::BlurPass)
              .expect("Renderpass not found.") }
    };
    RenderView::Config shadowmap_directional_view_config {
        "shadowmap_directional",
        Shader::BuiltIn::ShadowmapDirectionalShader,
        shadowmap_directional_size,
        shadowmap_directional_size,
        RenderView::Type::ShadowmapDirectional,
        RenderView::ViewMatrixSource::LightCamera,
        RenderView::ProjectionMatrixSource::DefaultOrthographic,
        { _app_renderer
              .get_renderpass(RenderPass::BuiltIn::ShadowmapDirectionalPass)
              .expect("Renderpass not found.") }
    };

    // Create
    const auto res_owv = _render_view_system.create(opaque_world_view_config);
    const auto res_uiv = _render_view_system.create(ui_view_config);
    const auto res_sbv = _render_view_system.create(skybox_view_config);
    const auto res_dev = _render_view_system.create(depth_view_config);
    const auto res_aov = _render_view_system.create(ao_view_config);
    const auto res_blv = _render_view_system.create(blur_view_config);
    const auto res_smdv =
        _render_view_system.create(shadowmap_directional_view_config);
    if (res_owv.has_error() || res_uiv.has_error() || res_sbv.has_error() ||
        res_dev.has_error() || res_aov.has_error() || res_blv.has_error() ||
        res_smdv.has_error())
        Logger::fatal("Render view creation failed.");

    _ow_render_view = dynamic_cast<RenderViewWorld*>(res_owv.value());
    _ui_render_view = dynamic_cast<RenderViewUI*>(res_uiv.value());
    _sb_render_view = dynamic_cast<RenderViewSkybox*>(res_sbv.value());
    _de_render_view = dynamic_cast<RenderViewDepth*>(res_dev.value());
    _ao_render_view = dynamic_cast<RenderViewAO*>(res_aov.value());
    _bl_render_view = dynamic_cast<RenderViewBlur*>(res_blv.value());
    _smd_render_view =
        dynamic_cast<RenderViewShadowmapDirectional*>(res_smdv.value());

    // Set render view textures
    _ssao_map =
        _app_renderer.create_texture_map({ blured_ssao_texture,
                                           Texture::Use::Unknown,
                                           Texture::Filter::BiLinear,
                                           Texture::Filter::BiLinear,
                                           Texture::Repeat::ClampToEdge,
                                           Texture::Repeat::ClampToEdge,
                                           Texture::Repeat::ClampToEdge });
    _ow_render_view->set_ssao_texture(_ssao_map);

    _shadowmap_directional_map =
        _app_renderer.create_texture_map({ shadowmap_directional_depth_texture,
                                           Texture::Use::Unknown,
                                           Texture::Filter::BiLinear,
                                           Texture::Filter::BiLinear,
                                           Texture::Repeat::ClampToEdge,
                                           Texture::Repeat::ClampToEdge,
                                           Texture::Repeat::ClampToEdge });
    _ow_render_view->set_shadowmap_directional_texture(
        _shadowmap_directional_map
    );
}

void TestApplication::setup_scene_geometry() {
    Vector<Mesh*> meshes {};
    Vector<Mesh*> ui_meshes {};

#define CURRENT_SCENE 2
#if CURRENT_SCENE == 0
    // Create geometries
    Geometry* const geometry_1 =
        _geometry_system.generate_cube("cube", "test_material");
    Geometry* const geometry_2 =
        _geometry_system.generate_cube("cube", "test_material");
    Geometry* const geometry_3 =
        _geometry_system.generate_cube("cube", "test_material");

    // Create meshes
    Mesh* mesh_1 = new (MemoryTag::Geometry) Mesh(geometry_1);
    Mesh* mesh_2 = new (MemoryTag::Geometry) Mesh(geometry_2);
    Mesh* mesh_3 = new (MemoryTag::Geometry) Mesh(geometry_3);

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
#else
    /// Load MESH TEST
    MeshLoader loader {};
#    if CURRENT_SCENE == 1
    auto load_result = loader.load("falcon");
#    elif CURRENT_SCENE == 2
    auto load_result = loader.load("sponza");
#    elif CURRENT_SCENE == 3
    auto load_result = loader.load("viking_room");
#    endif
    if (load_result.has_error()) {
        Logger::error(load_result.error().what());
        Logger::fatal("Mesh loading failed");
    }
    auto config_array = dynamic_cast<GeometryConfigArray*>(load_result.value());

    // Add all geometries
    Vector<Geometry*> geometries {};
    geometries.reserve(config_array->configs.size());
    for (auto config : config_array->configs)
        geometries.push_back(_geometry_system.acquire(*config));

    // Add mesh
    Mesh* mesh = new (MemoryTag::Geometry) Mesh(geometries);
#    if CURRENT_SCENE == 2 || CURRENT_SCENE == 1
#        if CURRENT_SCENE == 2
    mesh->transform.scale_by(0.02f);
    mesh->transform.translate_by(glm::vec3(0, -1, 0));
#        endif
    mesh->transform.rotate_by_deg(glm::vec3(1, 0, 0), 90.f);
#    endif
    meshes.push_back(mesh);
#endif

    /// Load GUI TEST
    // Create geometry
    const auto geom_2d = _geometry_system.generate_ui_rectangle(
        "ui", 128, 128, "test_ui_material"
    );

    /// Load skybox
    // Create geometry
    Geometry* const skybox_geometry =
        _geometry_system.generate_cube("cube", "");

    // Create texture map
    const auto skybox_map =
        _app_renderer.create_texture_map(Texture::Map::Config {
            _texture_system.acquire_cube("skybox/skybox", true),
            Texture::Use::MapCube,
            Texture::Filter::BiLinear,
            Texture::Filter::BiLinear,
            Texture::Repeat::Repeat,
            Texture::Repeat::Repeat,
            Texture::Repeat::Repeat });

    // Acquire instance resources
    const auto skybox_instance_id =
        _app_renderer.skybox_shader->acquire_instance_resources({ skybox_map });

    // Create skybox
    _default_skybox = { skybox_instance_id, skybox_map, skybox_geometry };

    // === Mesh render data ===
    // Create mesh
    Mesh* mesh_ui = new (MemoryTag::Geometry) Mesh(geom_2d);
    ui_meshes.push_back(mesh_ui);

    // Create mesh data
    _world_mesh_data = { meshes };
    _ui_mesh_data    = { ui_meshes };

    // Set mesh data
    _ow_render_view->set_render_data_ref(&_world_mesh_data);
    _ui_render_view->set_render_data_ref(&_ui_mesh_data);
    _sb_render_view->set_skybox_ref(&_default_skybox);
    _de_render_view->set_render_data_ref(&_world_mesh_data);
    _smd_render_view->set_render_data_ref(&_world_mesh_data);
}

void TestApplication::setup_lights() {
    _ow_render_view->set_light_system(&_light_system);
    _smd_render_view->set_light_system(&_light_system);

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