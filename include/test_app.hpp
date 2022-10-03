#pragma once

#include "logger.hpp"

#include "systems/geometry_system.hpp"

// TODO: TEMP
void load_model(std::vector<Vertex>& out_vertices, std::vector<uint32>& out_indices);

class TestApplication {
public:
    TestApplication();
    ~TestApplication();

    void run();

private:
    Platform::Surface* _app_surface = Platform::Surface::get_instance(800, 600, std::string(APP_NAME));
    Renderer _app_renderer{ RendererBackendType::Vulkan, _app_surface };

    TextureSystem _texture_system{ &_app_renderer };
    MaterialSystem _material_system{ &_app_renderer, &_texture_system };
    GeometrySystem _geometry_system{ &_app_renderer, &_material_system };

    float32 calculate_delta_time();
};

TestApplication::TestApplication() {}

TestApplication::~TestApplication() {}

void TestApplication::run() {
    std::vector<Vertex> vertices = {};
    std::vector<uint32> indices = {};
    load_model(vertices, indices);
    _app_renderer.current_geometry = _geometry_system.acquire(
        vertices,
        indices,
        "viking_room",
        "viking_room"
    );

    while (!_app_surface->should_close()) {
        _app_surface->process_events();

        auto delta_time = calculate_delta_time();

        _app_renderer.draw_frame(delta_time);
    }

    _app_renderer.~Renderer();
}

float32 TestApplication::calculate_delta_time() {
    static auto start_time = Platform::get_absolute_time();
    auto current_time = Platform::get_absolute_time();
    auto delta_time = current_time - start_time;
    start_time = current_time;
    return delta_time;
}

// TODO: TEMP MODEL LOADING LIBS
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

void load_model(std::vector<Vertex>& out_vertices, std::vector<uint32>& out_indices) {
    // Load model
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(model_path))
        throw std::runtime_error("");

    if (!reader.Warning().empty())
        Logger::warning("TinyObjReader :: ", reader.Warning());

    auto& attributes = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // Loop over shapes
    std::unordered_map<Vertex, uint32> unique_vertices = {};
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attributes.vertices[3 * index.vertex_index + 0],
                attributes.vertices[3 * index.vertex_index + 1],
                attributes.vertices[3 * index.vertex_index + 2]
            };

            vertex.texture_coord = {
                attributes.texcoords[2 * index.texcoord_index + 0],
                1.0f - attributes.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f,1.0f,1.0f };

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32>(out_vertices.size());
                out_vertices.push_back(vertex);
            }

            out_indices.push_back(unique_vertices[vertex]);
        }
    }
}