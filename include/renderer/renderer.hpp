#pragma once

#include "renderer/vulkan/vulkan_backend.hpp"

enum RendererBackendType {
    Vulkan
};

class Renderer {
public:
    Renderer(RendererBackendType backend_type, Platform::Surface* surface);
    ~Renderer();

    // TODO: TEMP TEST CODE
    Texture* current_texture = nullptr;

    void on_resize(const uint32 width, const uint32 height);
    bool draw_frame(const float32 delta_time);

    void create_texture(Texture* texture);
    void destroy_texture(Texture* texture);

private:
    RendererBackend* _backend = nullptr;

    float32 _near_plane = 0.01f;
    float32 _far_plane = 1000.0f;
    glm::mat4 _projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, _near_plane, _far_plane);
    glm::mat4 _view =
        glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
};