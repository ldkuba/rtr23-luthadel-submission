#ifndef __RENDERER_H__
#define __RENDERER_H__

#pragma once

#include "renderer/vulkan/vulkan_backend.hpp"

enum RendererBackendType {
    Vulkan
};

class Renderer {
public:
    Renderer(
        const RendererBackendType backend_type,
        Platform::Surface* const surface,
        ResourceSystem* const resource_system
    );
    ~Renderer();

    // Prevent accidental copying
    Renderer(Renderer const&) = delete;
    Renderer& operator = (Renderer const&) = delete;


    // TODO: TEMP TEST CODE
    Geometry* current_geometry = nullptr;

    void on_resize(const uint32 width, const uint32 height);
    bool draw_frame(const float32 delta_time);

    void create_texture(Texture* texture, const byte* const data);
    void destroy_texture(Texture* texture);

    void create_material(Material* const material);
    void destroy_material(Material* const material);

    void create_geometry(
        Geometry* geometry,
        const std::vector<Vertex> vertices,
        const std::vector<uint32> indices
    );
    void destroy_geometry(Geometry* geometry);

private:
    RendererBackend* _backend = nullptr;
    ResourceSystem* _resource_system = nullptr;

    float32 _near_plane = 0.01f;
    float32 _far_plane = 1000.0f;
    glm::mat4 _projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, _near_plane, _far_plane);
    glm::mat4 _view =
        glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
};
#endif // __RENDERER_H__