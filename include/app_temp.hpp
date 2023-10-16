#pragma once

#include "logger.hpp"

#include "math_libs.hpp"
#include "systems/geometry_system.hpp"
#include "systems/input/input_system.hpp"

// TODO: TEMP
#include "resources/loaders/mesh_loader.hpp"

namespace ENGINE_NAMESPACE {

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

    void setup_input();
};
// TODO: TEMP
inline TestApplication::TestApplication() {}

inline TestApplication::~TestApplication() { del(_app_surface); }

inline void TestApplication::run() {
    // === Input system ===
    setup_input();

    // === Renderer ===
    _app_renderer.material_shader =
        _shader_system.acquire("builtin.material_shader").expect("ERR1");
    _app_renderer.ui_shader =
        _shader_system.acquire("builtin.ui_shader").expect("ERR2");

    _app_renderer.material_shader->reload();

    // _app_renderer.current_geometry =
    //     _geometry_system.generate_cube("cube", "test_material");

    /// Load MESH TEST
    MeshLoader loader {};
    // auto       load_result = loader.load("falcon");
    // auto       load_result = loader.load("sponza");
    auto       load_result = loader.load("viking_room");
    if (load_result.has_error()) {
        Logger::error(load_result.error().what());
        Logger::fatal("Mesh loading failed");
    }
    auto config_array = dynamic_cast<GeometryConfigArray*>(load_result.value());
    GeometryConfig* config         = config_array->configs[0];
    _app_renderer.current_geometry = _geometry_system.acquire(*config);

    Logger::debug(
        "Diffuse map\t: ",
        _app_renderer.current_geometry->material()->diffuse_map().texture->name
    );
    Logger::debug(
        "Specular map\t: ",
        _app_renderer.current_geometry->material()->specular_map().texture->name
    );
    Logger::debug(
        "Normal map\t: ",
        _app_renderer.current_geometry->material()->normal_map().texture->name
    );

    /// Load GUI TEST
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

inline void TestApplication::setup_input() {
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

    // Camera: info
    auto& camera_p = _app_renderer.camera_position;
    auto& camera_d = _app_renderer.camera_look_dir;
    camera_d       = glm::normalize(camera_d);

    const static auto camera_u = glm::vec3(0.0f, 0.0f, 1.0f);

    static float32 camera_speed   = 5.0f;
    static float32 rotation_speed = 1.4f;

    // Camera: movement
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

    // Camera: rotation
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

    // Camera: other
    reset_camera->event += [&](float32, float32) {
        camera_p = glm::vec3(2, 2, 2);
        camera_d = glm::vec3(-1, -1, -1);
        camera_d = glm::normalize(camera_d);
    };
    camera_position->event +=
        [&](float32, float32) { Logger::debug(camera_p); };

    // Rendering
    auto& r = _app_renderer;
    mode_0_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Default; };
    mode_1_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Lighting; };
    mode_2_c->event +=
        [&r](float32, float32) { r.view_mode = DebugViewMode::Normals; };

    // Other
    spin_cube->event += [&](float32, float32) {
        _app_renderer.cube_rotation = !_app_renderer.cube_rotation;
    };
    shader_reload->event +=
        [&](float32, float32) { _app_renderer.material_shader->reload(); };
}

} // namespace ENGINE_NAMESPACE