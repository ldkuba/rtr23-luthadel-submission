#pragma once

#include "renderer/vulkan/vulkan_backend.hpp"

enum RendererBackendType {
    Vulkan
};

class Renderer {
private:
    RendererBackend* _backend;

public:
    Renderer(RendererBackendType backend_type, Platform::Surface* surface);
    ~Renderer();

    void on_resize(uint32 width, uint32 height);
    bool draw_frame(float32 delta_time);

    void wait_for_shutdown();
};