#include <iostream>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "logger.hpp"

const char* NAME = "Vulkan Engine";
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

GLFWwindow* window;
vk::Instance vulkan_instance;

void initVulkan() {
    // Optional info
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = NAME;
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = NAME;
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_1; // Set vulkan version

    // GLFW window extension
    uint32 glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Mandatory info
    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

    // Create instance
    vk::Result result = vk::createInstance(&createInfo, nullptr, &vulkan_instance);
    if (result != vk::Result::eSuccess) {
        Logger::fatal("failed to create VULKAN instance!");
        std::exit(1);
    }

    // Get extension data
    uint32 extension_count = 0;
    result = vk::enumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    std::vector<vk::ExtensionProperties> extensions(extension_count);
    result = vk::enumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
    // for (const auto& extension : extensions) std::cout << '\t' << extension.extensionName << '\n';

}
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}

void cleanup() {
    vulkan_instance.destroy();

}

#include "test_app.hpp"


int main(int, char**) {

    TestApplication app{};

    try {
        app.run();
    } catch (const std::exception& e) {
        Logger::fatal(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}