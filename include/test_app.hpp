#pragma once

#include "logger.hpp"

#include "renderer/renderer.hpp"
#include "systems/texture_system.hpp"

#include <chrono>

class TestApplication {
public:
    TestApplication();
    ~TestApplication();

    void run();

private:
    Platform::Surface* _app_surface = Platform::Surface::get_instance(800, 600, std::string(APP_NAME));
    Renderer _app_renderer{ RendererBackendType::Vulkan, _app_surface };

    TextureSystem _texture_system{ &_app_renderer };

    float32 calculate_delta_time();
};

TestApplication::TestApplication() {}

TestApplication::~TestApplication() {}

void TestApplication::run() {
    _app_renderer.current_texture = _texture_system.default_texture;
    _app_renderer.current_texture = _texture_system.acquire("viking_room", true);

    while (!_app_surface->should_close()) {
        _app_surface->process_events();

        auto delta_time = calculate_delta_time();

        _app_renderer.draw_frame(delta_time);
    }

    _app_renderer.~Renderer();
}

float32 TestApplication::calculate_delta_time() {
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto current_time = std::chrono::high_resolution_clock::now();
    auto delta_time = std::chrono::duration<float32, std::chrono::seconds::period>
        (current_time - start_time).count();
    start_time = current_time;
    return delta_time;
}