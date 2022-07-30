
#include "window.hpp"
#include "pipeline.hpp"

class TestApplication {
private:
    Window _app_window{ 800, 600, "Vulkan Engine" };
    Pipeline _app_pipeline{ "build/shaders/simple_vertex_shader.vert.spv", "build/shaders/simple_fragment_shader.frag.spv" };


public:
    TestApplication(/* args */);
    ~TestApplication();

    void run();
};

TestApplication::TestApplication(/* args */) {}

TestApplication::~TestApplication() {}

void TestApplication::run() {
    while (!_app_window.should_close()) {
        glfwPollEvents();
    }

}