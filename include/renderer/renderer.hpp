#pragma once

#include "renderer/vulkan/vulkan_backend.hpp"

enum RendererBackendType {
    Vulkan
};

class Renderer {
public:
    Renderer(RendererBackendType backend_type, Platform::Surface* surface);
    ~Renderer();

    void on_resize(const uint32 width, const uint32 height);
    bool draw_frame(const float32 delta_time);

private:
    RendererBackend* _backend;
};