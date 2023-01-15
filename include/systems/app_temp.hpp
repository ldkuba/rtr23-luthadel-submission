#pragma once

#include "logger.hpp"

#include "math_libs.hpp"
#include "systems/geometry_system.hpp"
#include "systems/input/input_system.hpp"

void load_model(Vector<Vertex>& out_vertices, Vector<uint32>& out_indices);

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
    void setup_camera_controls();
};
// TODO: TEMP
void load_model(Vector<Vertex>& out_vertices, Vector<uint32>& out_indices);

inline TestApplication::TestApplication() {}

inline TestApplication::~TestApplication() { delete _app_surface; }

inline void TestApplication::run() {
    // === Input system ===
    _input_system.register_input_source(_app_surface);

    auto close_app_control =
        _input_system.create_control("Close app", ControlType::Release)
            .expect("ERROR :: CONTROL CREATION FAILED.");

    bool close_required = false;
    close_app_control->event +=
        [&close_required](float64, float64) { close_required = true; };
    close_app_control->map_key(KeyCode::ESCAPE);

    setup_camera_controls();

    // === Renderer ===
    _app_renderer.material_shader =
        _shader_system.acquire("builtin.material_shader").expect("ERR1");
    _app_renderer.ui_shader =
        _shader_system.acquire("builtin.ui_shader").expect("ERR2");

    bool use_cube = true;
    if (use_cube) {
        _app_renderer.current_geometry =
            _geometry_system.generate_cube("cube", "viking_room");
    } else {
        Vector<Vertex> vertices = {};
        Vector<uint32> indices  = {};
        load_model(vertices, indices);
        _app_renderer.current_geometry = _geometry_system.acquire(
            "viking_room", vertices, indices, "viking_room"
        );
    }

    float32          side       = 128.0f;
    Vector<Vertex2D> vertices2d = {
        { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
        { glm::vec2(side, side), glm::vec2(1.0f, 1.0f) },
        { glm::vec2(0.0f, side), glm::vec2(0.0f, 1.0f) },
        { glm::vec2(side, 0.0f), glm::vec2(1.0f, 0.0f) }
    };
    Vector<uint32> indices2d          = { 2, 1, 0, 3, 0, 1 };
    _app_renderer.current_ui_geometry = _geometry_system.acquire(
        "ui", vertices2d, indices2d, "test_ui_material"
    );

    // === Main loop ===
    while (!_app_surface->should_close() && close_required == false) {
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

// TODO: TEMP MODEL LOADING LIBS
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "unordered_map.hpp"

inline void load_model(
    Vector<Vertex>& out_vertices, Vector<uint32>& out_indices
) {
    // Load model
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile("../assets/models/viking_room.obj"))
        throw std::runtime_error("");

    if (!reader.Warning().empty())
        Logger::warning("TinyObjReader :: ", reader.Warning());

    auto& attributes = reader.GetAttrib();
    auto& shapes     = reader.GetShapes();
    auto& materials  = reader.GetMaterials();

    // Loop over shapes
    UnorderedMap<Vertex, uint32> unique_vertices = {};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex {};

            // Compute position
            vertex.position = {
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2]
            };

            // Compute normal
            vertex.normal = { attributes.normals[3 * index.normal_index + 0],
                              attributes.normals[3 * index.normal_index + 1],
                              attributes.normals[3 * index.normal_index + 2] };

            // Compute texture coordinate
            vertex.texture_coord = {
                attributes.texcoords[2 * index.texcoord_index + 0],
                1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
            };

            // Compute index
            if (!unique_vertices.contains(vertex)) {
                unique_vertices[vertex] =
                    static_cast<uint32>(out_vertices.size());
                out_vertices.push_back(vertex);
            }

            out_indices.push_back(unique_vertices[vertex]);
        }
    }
}

#define HoldControl(name)                                                      \
    auto name = _input_system.create_control(#name, ControlType::Hold)         \
                    .expect("ERROR :: CONTROL CREATION FAILED.");

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
    auto reset_camera =
        _input_system.create_control("reset_camera", ControlType::Release)
            .expect("ERROR :: CONTROL CREATION FAILED.");

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
}