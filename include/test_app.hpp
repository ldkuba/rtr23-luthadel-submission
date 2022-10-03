#pragma once

#include "logger.hpp"

#include "renderer/renderer.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"

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

    float32 calculate_delta_time();
};

TestApplication::TestApplication() {}

TestApplication::~TestApplication() {}

void TestApplication::run() {
    _app_renderer.current_material = _material_system.acquire(
        "viking_room",
        true,
        glm::vec4(1.0f),
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