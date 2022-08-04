#include "renderer/vulkan/vulkan_backend.hpp"

#include <string>
#include <optional>

#include "logger.hpp"

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
);


VulkanBackend::VulkanBackend(Platform::Surface* surface) : RendererBackend(surface) {
    create_vulkan_instance();
    setup_debug_messenger();
    _vulkan_surface = _surface->get_vulkan_surface(_vulkan_instance, _allocator);
    // _device = new VulkanDevice(&_vulkan_instance);
}

VulkanBackend::~VulkanBackend() {
    delete _device;
    _vulkan_instance.destroySurfaceKHR(_vulkan_surface, nullptr);
    if (_enable_validation_layers)
        _vulkan_instance.destroyDebugUtilsMessengerEXT(_debug_messenger, nullptr,
            vk::DispatchLoaderDynamic{ _vulkan_instance, vkGetInstanceProcAddr });
    _vulkan_instance.destroy();
}

// ///////////////////////////////// //
// VULKAN RENDERER PRIVATE FUNCTIONS //
// ///////////////////////////////// //

void VulkanBackend::create_vulkan_instance() {
    if (_enable_validation_layers && !all_validation_layers_are_available())
        throw std::runtime_error("Validation layer was requested, but not available.");

    // Optional application info
    vk::ApplicationInfo app_info{};
    app_info.pApplicationName = APP_NAME;       // Application name
    app_info.applicationVersion = 1;            // Application version
    app_info.pEngineName = APP_NAME;            // Engine name
    app_info.engineVersion = 1;                 // Engine version
    app_info.apiVersion = VK_API_VERSION_1_2;   // Set vulkan version

    // Mandatory vulkan info info
    vk::InstanceCreateInfo create_info{};
    create_info.pApplicationInfo = &app_info;

    // Specifying required extensions:
    // - Platform extensions
    // - Debug message extension
    auto extensions = Platform::get_required_vulkan_extensions();
    if (_enable_validation_layers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    create_info.enabledExtensionCount = static_cast<uint32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    // Validation layers debugging
    vk::DebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (_enable_validation_layers) {
        debug_create_info = debug_messenger_create_info();
        create_info.pNext = (vk::DebugUtilsMessengerCreateInfoEXT*) &debug_create_info;
        create_info.enabledLayerCount = static_cast<uint32>(_validation_layers.size());
        create_info.ppEnabledLayerNames = _validation_layers.data();
    } else create_info.enabledLayerCount = 0;

    // Create instance
    vk::Result result = vk::createInstance(&create_info, _allocator, &_vulkan_instance);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create VULKAN instance.");

#define PRINT_EXTENSIONS 0
#if PRINT_EXTENSIONS
    // Get extension data
    auto extensions = vk::enumerateInstanceExtensionProperties();
    for (const auto& extension : extensions) std::cout << '\t' << extension.extensionName << '\n';
#endif
}

void VulkanBackend::setup_debug_messenger() {
    if (!_enable_validation_layers) return;

    auto create_info = debug_messenger_create_info();

    // Dispatcher needed for vulkan extension functions
    auto dispatcher = vk::DispatchLoaderDynamic{ _vulkan_instance, vkGetInstanceProcAddr };

    // Create debugger
    vk::Result result = _vulkan_instance.createDebugUtilsMessengerEXT(&create_info, nullptr, &_debug_messenger, dispatcher);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to set up debug messenger!");
}


bool VulkanBackend::all_validation_layers_are_available() {
    auto available_layers = vk::enumerateInstanceLayerProperties();

    for (auto validation_layer : _validation_layers) {
        bool validation_layer_available = false;
        for (auto layer : available_layers) {
            if (std::strcmp(layer.layerName, validation_layer) == 0) {
                validation_layer_available = true;
                break;
            }
        }
        if (validation_layer_available == false)
            return false;
    }

    return true;
}

vk::DebugUtilsMessengerCreateInfoEXT VulkanBackend::debug_messenger_create_info() {
    vk::DebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.setMessageSeverity(                             // Severity levels enabled
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
    );
    create_info.setMessageType(                                 // Message types enabled
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );
    create_info.setPfnUserCallback(debug_callback_function);    // User defined debug callback
    create_info.setPUserData(nullptr);                          // (Optional) Additional user data (can be anything)

    return create_info;
}

// //////////////////////////////// //
// VULKAN RENDERER PUBLIC FUNCTIONS //
// //////////////////////////////// //

void VulkanBackend::resized(uint32 width, uint32 height) {}
bool VulkanBackend::begin_frame(float32 delta_time) {
    return true;
}
bool VulkanBackend::end_frame(float32 delta_time) {
    return true;
}

// //////////////// //
// Helper functions //
// //////////////// //

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback_function(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data
) {
    std::string message = "VULKAN :: " + std::string(callback_data->pMessage);
    switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        Logger::log(message);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        Logger::verbose(message);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        Logger::warning(message);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        if (message_type == VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
            Logger::fatal(message);
        else
            Logger::error(message);
        break;
    default:
        break;
    }

    return VK_FALSE;
}