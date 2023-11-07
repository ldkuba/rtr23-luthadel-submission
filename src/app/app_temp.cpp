#include "app/app_temp.hpp"

#include "resources/loaders/mesh_loader.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
TestApplication::TestApplication() {}
TestApplication::~TestApplication() { delete _app_surface; }

// /////////////////////// //
// APP TEMP PUBLIC METHODS //
// /////////////////////// //

void TestApplication::run() {
    // === Camera system ===
    main_camera                     = _camera_system.default_camera;
    main_camera->transform.position = glm::vec3(2, 2, 2);
    main_camera->add_pitch(35);
    main_camera->add_yaw(-135);
    _app_renderer.set_active_camera(main_camera);

    // === Input system ===
    setup_input();

    // === Renderer ===
    _app_renderer.material_shader =
        _shader_system.acquire("builtin.material_shader").expect("ERR1");
    _app_renderer.ui_shader =
        _shader_system.acquire("builtin.ui_shader").expect("ERR2");

    _app_renderer.material_shader->reload();

    // === Assign meshes ===
    Vector<Mesh*> meshes {};

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
    Mesh* mesh_1 = new Mesh(geometry_1);
    Mesh* mesh_2 = new Mesh(geometry_2);
    Mesh* mesh_3 = new Mesh(geometry_3);

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
    auto       load_result = loader.load("falcon");
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
    Mesh* mesh = new (MemoryTag::Temp) Mesh(geometries);
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
    float32          side = 128.0f;
    Vector<Vertex2D> vertices2d {
        { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec2(side, side), glm::vec2(1.0f, 1.0f) },
        { glm::vec2(0.0f, side), glm::vec2(0.0f, 1.0f) },
        { glm::vec2(side, 0.0f), glm::vec2(1.0f, 0.0f) }
    };
    Vector<uint32>   indices2d { 2, 1, 0, 3, 0, 1 };
    GeometryConfig2D config2d { "ui",
                                vertices2d,
                                indices2d,
                                glm::vec3(side / 2.0f),
                                glm::vec3(side),
                                glm::vec3(0),
                                "test_ui_material" };
    const auto       geom_2d = _geometry_system.acquire(config2d);

    // === Construct render packet ===
    RenderPacket packet {};

    // Add 3D meshes
    for (const auto mesh : meshes)
        mesh->add_geometry_to_render_packet(packet);

    // Add UI
    packet.ui_geometry_data.geometry = geom_2d;
    packet.ui_geometry_data.model    = glm::mat4(1.f);

    // === Main loop ===
    while (!_app_surface->should_close() && _app_should_close == false) {
        auto delta_time = calculate_delta_time();

        _app_surface->process_events(delta_time);

        // TODO: Temp code; update one and only object
        if (_cube_rotation) {
            float rotation_speed = 1.0f * delta_time;
            for (auto& mesh : meshes) {
                mesh->transform.rotate_by(
#if CURRENT_SCENE == 2 || CURRENT_SCENE == 1
                    glm::vec3(0.0f, 1.0f, 0.0f),
#else
                    glm::vec3(0.0f, 0.0f, 1.0f),
#endif
                    rotation_speed
                );
            }

            // Readd everything
            packet.geometry_data.clear();
            for (const auto mesh : meshes)
                mesh->add_geometry_to_render_packet(packet);
        }

        auto result = _app_renderer.draw_frame(&packet, delta_time);
        if (result.has_error()) {
            // TODO: PROCESS ERROR
            Logger::error(result.error().what());
        }
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

// TODO: Still temp

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

    // === Events ===
    // Application controls
    close_app_control->event +=
        [&](float64, float64) { _app_should_close = true; };

    // Camera: info, TODO: TEMP
    static const float32 camera_speed   = 5.0f;
    static const float32 rotation_speed = 100.0f;

    // Camera: movement
    camera_forward_c->event += [&](float32 dt, float32) {
        main_camera->move_forwards(camera_speed * dt);
    };
    camera_backwards_c->event += [&](float32 dt, float32) {
        main_camera->move_backwards(camera_speed * dt);
    };
    camera_left_c->event += [&](float32 dt, float32) { //
        main_camera->move_left(camera_speed * dt);
    };
    camera_right_c->event += [&](float32 dt, float32) {
        main_camera->move_right(camera_speed * dt);
    };
    camera_up_c->event +=
        [&](float32 dt, float32) { main_camera->move_up(camera_speed * dt); };
    camera_down_c->event +=
        [&](float32 dt, float32) { main_camera->move_down(camera_speed * dt); };

    // Camera: rotation
    camera_rotate_right_c->event += [&](float32 dt, float32) { //
        main_camera->add_yaw(-rotation_speed * dt);
    };
    camera_rotate_left_c->event += [&](float32 dt, float32) { //
        main_camera->add_yaw(rotation_speed * dt);
    };
    camera_rotate_up_c->event += [&](float32 dt, float32) {
        main_camera->add_pitch(-rotation_speed * dt);
    };
    camera_rotate_down_c->event += [&](float32 dt, float32) {
        main_camera->add_pitch(rotation_speed * dt);
    };

    // Camera: other
    reset_camera->event += [&](float32, float32) {
        main_camera->reset();
        main_camera->transform.position = glm::vec3(2, 2, 2);
        main_camera->add_pitch(35);
        main_camera->add_yaw(-135);
    };
    camera_position->event += [&](float32, float32) {
        Logger::debug(main_camera->transform.position());
    };

    // Rendering
    auto& r = _app_renderer;
    mode_0_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Default; };
    mode_1_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Lighting; };
    mode_2_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Normals; };

    // Other
    spin_cube->event +=
        [&](float32, float32) { _cube_rotation = !_cube_rotation; };
    shader_reload->event +=
        [&](float32, float32) { _app_renderer.material_shader->reload(); };
}

} // namespace ENGINE_NAMESPACE