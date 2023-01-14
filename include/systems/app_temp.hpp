#pragma once

#include "logger.hpp"

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

    // Camera controls

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