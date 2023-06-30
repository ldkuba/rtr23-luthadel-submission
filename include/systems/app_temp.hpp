#pragma once

#include "logger.hpp"

#include "math_libs.hpp"
#include "systems/geometry_system.hpp"
#include "systems/input/input_system.hpp"

// TODO: TEMP
#include "resources/loaders/mesh_loader.hpp"

class TestApplication {
  public:
    TestApplication();
    ~TestApplication();

    void run();

  private:
    Platform::Surface* _app_surface =
        Platform::Surface::get_instance(800, 600, std::string(APP_NAME));

    InputSystem    _input_system {};
    ResourceSystem _resource_system {};

    Renderer _app_renderer { RendererBackendType::Vulkan, _app_surface };

    TextureSystem  _texture_system { &_app_renderer, &_resource_system };
    ShaderSystem   _shader_system { &_app_renderer,
                                  &_resource_system,
                                  &_texture_system };
    MaterialSystem _material_system {
        &_app_renderer, &_resource_system, &_texture_system, &_shader_system
    };
    GeometrySystem _geometry_system { &_app_renderer, &_material_system };

    float64 calculate_delta_time();

    // TODO: TEMP
    bool _app_should_close = false;

    void setup_application_controls();
    void setup_camera_controls();
    void setup_render_mode_controls();
};
// TODO: TEMP
inline TestApplication::TestApplication() {}

inline TestApplication::~TestApplication() { delete _app_surface; }

inline void TestApplication::run() {
    // === Input system ===
    _input_system.register_input_source(_app_surface);
    setup_application_controls();
    setup_camera_controls();
    setup_render_mode_controls();

    // Cube spin
    auto spin_cube =
        _input_system.create_control("spin_cube", ControlType::Press)
            .expect("ERROR :: CONTROL CREATION FAILED.");
    spin_cube->event += [&](float32, float32) {
        _app_renderer.cube_rotation = !_app_renderer.cube_rotation;
    };
    spin_cube->map_key(KeyCode::SPACE);

    // Material shader reload
    auto shader_reload =
        _input_system.create_control("shader_reload", ControlType::Press)
            .expect("ERROR :: CONTROL CREATION FAILED.");
    shader_reload->event +=
        [&](float32, float32) { _app_renderer.material_shader->reload(); };
    shader_reload->map_key(KeyCode::Z);

    // === Renderer ===
    _app_renderer.material_shader =
        _shader_system.acquire("builtin.material_shader").expect("ERR1");
    _app_renderer.ui_shader =
        _shader_system.acquire("builtin.ui_shader").expect("ERR2");

    _app_renderer.material_shader->reload();

    _app_renderer.current_geometry =
        _geometry_system.generate_cube("cube", "test_material");

    // MeshLoader loader {};
    // auto       load_result = loader.load("viking_room");
    // if (load_result.has_error()) Logger::fatal("Mesh loading failed");
    // auto config_array =
    // dynamic_cast<GeometryConfigArray*>(load_result.value()); GeometryConfig*
    // config         = config_array->configs[0]; config->material_name =
    // "cobblestone"; _app_renderer.current_geometry =
    // _geometry_system.acquire(*config);

    float32          side       = 128.0f;
    Vector<Vertex2D> vertices2d = {
        { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec2(side, side), glm::vec2(1.0f, 1.0f) },
        { glm::vec2(0.0f, side), glm::vec2(0.0f, 1.0f) },
        { glm::vec2(side, 0.0f), glm::vec2(1.0f, 0.0f) }
    };
    Vector<uint32>   indices2d = { 2, 1, 0, 3, 0, 1 };
    GeometryConfig2D config2d { "ui",
                                vertices2d,
                                indices2d,
                                glm::vec3(side / 2.0f),
                                glm::vec3(side),
                                glm::vec3(0),
                                "test_ui_material" };
    _app_renderer.current_ui_geometry = _geometry_system.acquire(config2d);

    // === Main loop ===
    while (!_app_surface->should_close() && _app_should_close == false) {
        auto delta_time = calculate_delta_time();

        _app_surface->process_events(delta_time);

        auto result = _app_renderer.draw_frame(delta_time);
        if (result.has_error()) {
            // TODO: PROCESS ERROR
            Logger::error(result.error().what());
        }
    }
}

inline float64 TestApplication::calculate_delta_time() {
    static auto start_time   = Platform::get_absolute_time();
    auto        current_time = Platform::get_absolute_time();
    auto        delta_time   = current_time - start_time;
    start_time               = current_time;
    return delta_time;
}

// TODO: Still temp
#define HoldControl(name)                                                      \
    auto name = _input_system.create_control(#name, ControlType::Hold)         \
                    .expect("ERROR :: CONTROL CREATION FAILED.");
#define PressControl(name)                                                     \
    auto name = _input_system.create_control(#name, ControlType::Press)        \
                    .expect("ERROR :: CONTROL CREATION FAILED.");
#define ReleaseControl(name)                                                   \
    auto name = _input_system.create_control(#name, ControlType::Release)      \
                    .expect("ERROR :: CONTROL CREATION FAILED.");

inline void TestApplication::setup_application_controls() {
    ReleaseControl(close_app_control);

    close_app_control->event +=
        [&](float64, float64) { _app_should_close = true; };

    close_app_control->map_key(KeyCode::ESCAPE);
}

inline void TestApplication::setup_camera_controls() {
    // Camera controls
    HoldControl(camera_forward_c);
    HoldControl(camera_backwards_c);
    HoldControl(camera_left_c);
    HoldControl(camera_right_c);
    HoldControl(camera_up_c);
    HoldControl(camera_down_c);
    HoldControl(camera_rotate_left_c);
    HoldControl(camera_rotate_right_c);
    HoldControl(camera_rotate_up_c);
    HoldControl(camera_rotate_down_c);
    ReleaseControl(reset_camera);
    ReleaseControl(camera_position);

    // Camera info
    auto& camera_p = _app_renderer.camera_position;
    auto& camera_d = _app_renderer.camera_look_dir;
    camera_d       = glm::normalize(camera_d);

    const static auto camera_u = glm::vec3(0.0f, 0.0f, 1.0f);

    static float32 camera_speed   = 5.0f;
    static float32 rotation_speed = 1.4f;

    // Camera movement
    camera_forward_c->event +=
        [&](float32 dt, float32) { camera_p += camera_d * camera_speed * dt; };
    camera_backwards_c->event +=
        [&](float32 dt, float32) { camera_p -= camera_d * camera_speed * dt; };
    camera_left_c->event += [&](float32 dt, float32) {
        auto camera_l = glm::cross(camera_u, camera_d);
        camera_l      = glm::normalize(camera_l);
        camera_p += camera_l * camera_speed * dt;
    };
    camera_right_c->event += [&](float32 dt, float32) {
        auto camera_l = glm::cross(camera_u, camera_d);
        camera_l      = glm::normalize(camera_l);
        camera_p -= camera_l * camera_speed * dt;
    };
    camera_up_c->event +=
        [&](float32 dt, float32) { camera_p += camera_u * camera_speed * dt; };
    camera_down_c->event +=
        [&](float32 dt, float32) { camera_p -= camera_u * camera_speed * dt; };

    // Camera rotation
    camera_rotate_left_c->event += [&](float32 dt, float32) {
        auto rot_mat =
            glm::rotate(glm::mat4(1.0f), rotation_speed * dt, camera_u);
        camera_d = rot_mat * glm::vec4(camera_d, 1.0f);
    };
    camera_rotate_right_c->event += [&](float32 dt, float32) {
        auto rot_mat =
            glm::rotate(glm::mat4(1.0f), -rotation_speed * dt, camera_u);
        camera_d = rot_mat * glm::vec4(camera_d, 1.0f);
    };
    camera_rotate_up_c->event += [&](float32 dt, float32) {
        auto camera_l = glm::cross(camera_d, camera_u);
        auto rot_mat =
            glm::rotate(glm::mat4(1.0f), rotation_speed * dt, camera_l);
        camera_d = rot_mat * glm::vec4(camera_d, 1.0f);
    };
    camera_rotate_down_c->event += [&](float32 dt, float32) {
        auto camera_l = glm::cross(camera_d, camera_u);
        auto rot_mat =
            glm::rotate(glm::mat4(1.0f), -rotation_speed * dt, camera_l);
        camera_d = rot_mat * glm::vec4(camera_d, 1.0f);
    };
    // Other
    reset_camera->event += [&](float32, float32) {
        camera_p = glm::vec3(2, 2, 2);
        camera_d = glm::vec3(-1, -1, -1);
        camera_d = glm::normalize(camera_d);
    };
    camera_position->event +=
        [&](float32, float32) { Logger::debug(camera_p); };

    // key bindings
    camera_forward_c->map_key(KeyCode::W);
    camera_backwards_c->map_key(KeyCode::S);
    camera_left_c->map_key(KeyCode::A);
    camera_right_c->map_key(KeyCode::D);
    camera_up_c->map_key(KeyCode::E);
    camera_down_c->map_key(KeyCode::Q);

    camera_rotate_left_c->map_key(KeyCode::J);
    camera_rotate_right_c->map_key(KeyCode::L);
    camera_rotate_up_c->map_key(KeyCode::I);
    camera_rotate_down_c->map_key(KeyCode::K);

    reset_camera->map_key(KeyCode::R);
    camera_position->map_key(KeyCode::C);
}

inline void TestApplication::setup_render_mode_controls() {
    PressControl(mode_0_c);
    PressControl(mode_1_c);
    PressControl(mode_2_c);
    PressControl(mode_3_c);
    PressControl(mode_4_c);
    PressControl(mode_5_c);
    PressControl(mode_6_c);

    auto& r = _app_renderer;
    mode_0_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Default; };
    mode_1_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Lighting; };
    mode_2_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Normals; };

    mode_0_c->map_key(KeyCode::NUM_0);
    mode_1_c->map_key(KeyCode::NUM_1);
    mode_2_c->map_key(KeyCode::NUM_2);
    mode_3_c->map_key(KeyCode::NUM_3);
    mode_4_c->map_key(KeyCode::NUM_4);
    mode_5_c->map_key(KeyCode::NUM_5);
    mode_6_c->map_key(KeyCode::NUM_6);
}