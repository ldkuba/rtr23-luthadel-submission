#pragma once

#include "platform/platform.hpp"
#include "math_libs.hpp"

#include "resources/material.hpp"

class RendererBackend {
public:
    RendererBackend(Platform::Surface* surface) {}
    ~RendererBackend() {}

    void increment_frame_number() { _frame_number++; }
    virtual void resized(const uint32 width, const uint32 height) {}
    virtual bool begin_frame(const float32 delta_time) { return false; }
    virtual bool end_frame(const float32 delta_time) { return false; }
    virtual void update_global_state(
        const glm::mat4 projection,
        const glm::mat4 view,
        const glm::vec3 view_position,
        const glm::vec4 ambient_color,
        const int32 mode
    ) {}
    virtual void update_object(
        const GeometryRenderData data
    ) {}

    virtual void create_texture(Texture* texture, const byte* const data) {}
    virtual void destroy_texture(Texture* texture) {}

    virtual void create_material(Material* const material) {}
    virtual void destroy_material(Material* const material) {}

private:
    uint64 _frame_number = 0;
};
