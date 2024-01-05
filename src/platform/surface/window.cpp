#include "platform/platform.hpp"
#if PLATFORM == WINDOWS32 || PLATFORM == LINUX

#    include "platform/surface/window.hpp"
#    include "systems/input/input_system.hpp"

namespace ENGINE_NAMESPACE {

// Platform overloads
Platform::Surface* Platform::Surface::get_instance(
    uint32 width, uint32 height, std::string name
) {
    return new (MemoryTag::Surface) Window(width, height, name);
}
const Vector<const char*> Platform::get_required_vulkan_extensions() {
    uint32_t count;
    auto     extensions = glfwGetRequiredInstanceExtensions(&count);
    return Vector<const char*>(extensions, extensions + count);
}

// Constructor & Destructor
Window::Window(int32 width, int32 height, std::string name)
    : _width(width), _height(height), _name(name) {
    // initialize GLFW with parameters
    glfwInit(); // Initialize GLFW
    glfwWindowHint(
        GLFW_CLIENT_API, GLFW_NO_API
    ); // We dont want a OpenGL context

    // Create window :: width, height, window name, monitor, share (OpenGL only)
    _window =
        glfwCreateWindow(_width, _height, _name.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(_window, this);
    // Setup resize callback
    glfwSetFramebufferSizeCallback(_window, framebuffer_resize_callback);
    // Setup key input callback
    glfwSetKeyCallback(_window, key_callback);
}
Window::~Window() {
    glfwDestroyWindow(_window);
    glfwTerminate();
}

// ///////////////////// //
// WINDOW PUBLIC METHODS //
// ///////////////////// //

Result<vk::SurfaceKHR, RuntimeError> Window::get_vulkan_surface(
    const vk::Instance&                  vulkan_instance,
    const vk::AllocationCallbacks* const allocator
) const {
    vk::SurfaceKHR vulkan_surface;
    vk::Result     result = static_cast<vk::Result>(glfwCreateWindowSurface(
        vulkan_instance,
        _window,
        (VkAllocationCallbacks*) allocator,
        (VkSurfaceKHR*) &vulkan_surface
    ));
    if (result != vk::Result::eSuccess)
        return Failure("Failed to create window surface.");
    return vulkan_surface;
}

uint32 Window::get_width_in_pixels() {
    int32 width;
    glfwGetFramebufferSize(_window, &width, nullptr);
    return static_cast<uint32>(width);
}
uint32 Window::get_height_in_pixels() {
    int32 height;
    glfwGetFramebufferSize(_window, nullptr, &height);
    return static_cast<uint32>(height);
}

void Window::process_events(const float64 delta_time) {
    glfwPollEvents();
    _input_system->invoke_held_keys(delta_time);
}

// ////////////////////// //
// WINDOW PRIVATE METHODS //
// ////////////////////// //

void Window::framebuffer_resize_callback(
    GLFWwindow* window, int32 width, int32 height
) {
    auto surface = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    surface->resize_event(std::max(width, 1), std::max(height, 1));
}

KeyCode* setup_key_code_translator();

void Window::key_callback(
    GLFWwindow* window, int key, int scancode, int action, int mods
) {
    static KeyCode* translator = setup_key_code_translator();
    auto surface = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    const KeyCode key_code = translator[key];

    switch (action) {
    case GLFW_PRESS: surface->_input_system->press_key(key_code); break;
    case GLFW_RELEASE: surface->_input_system->release_key(key_code); break;
    }
}

// /////////////////////// //
// WINDOW HELPER FUNCTIONS //
// /////////////////////// //

KeyCode* setup_key_code_translator() {
    KeyCode* translator = new (MemoryTag::Application) KeyCode[GLFW_KEY_LAST];

    // Number pad
    translator[GLFW_KEY_NUM_LOCK]    = KeyCode::NUM_LOCK;
    translator[GLFW_KEY_KP_0]        = KeyCode::NUM_PAD_0;
    translator[GLFW_KEY_KP_1]        = KeyCode::NUM_PAD_1;
    translator[GLFW_KEY_KP_2]        = KeyCode::NUM_PAD_2;
    translator[GLFW_KEY_KP_3]        = KeyCode::NUM_PAD_3;
    translator[GLFW_KEY_KP_4]        = KeyCode::NUM_PAD_4;
    translator[GLFW_KEY_KP_5]        = KeyCode::NUM_PAD_5;
    translator[GLFW_KEY_KP_6]        = KeyCode::NUM_PAD_6;
    translator[GLFW_KEY_KP_7]        = KeyCode::NUM_PAD_7;
    translator[GLFW_KEY_KP_8]        = KeyCode::NUM_PAD_8;
    translator[GLFW_KEY_KP_9]        = KeyCode::NUM_PAD_9;
    translator[GLFW_KEY_KP_ADD]      = KeyCode::NUM_PAD_ADD;
    translator[GLFW_KEY_KP_SUBTRACT] = KeyCode::NUM_PAD_SUBTRACT;
    translator[GLFW_KEY_KP_MULTIPLY] = KeyCode::NUM_PAD_MULTIPLY;
    translator[GLFW_KEY_KP_DIVIDE]   = KeyCode::NUM_PAD_DIVIDE;
    translator[GLFW_KEY_KP_DECIMAL]  = KeyCode::NUM_PAD_DECIMAL;
    translator[GLFW_KEY_KP_EQUAL]    = KeyCode::NUM_PAD_EQUAL;
    translator[GLFW_KEY_KP_ENTER]    = KeyCode::NUM_PAD_ENTER;

    // Top row numbers
    translator[GLFW_KEY_0]            = KeyCode::NUM_0;
    translator[GLFW_KEY_1]            = KeyCode::NUM_1;
    translator[GLFW_KEY_2]            = KeyCode::NUM_2;
    translator[GLFW_KEY_3]            = KeyCode::NUM_3;
    translator[GLFW_KEY_4]            = KeyCode::NUM_4;
    translator[GLFW_KEY_5]            = KeyCode::NUM_5;
    translator[GLFW_KEY_6]            = KeyCode::NUM_6;
    translator[GLFW_KEY_7]            = KeyCode::NUM_7;
    translator[GLFW_KEY_8]            = KeyCode::NUM_8;
    translator[GLFW_KEY_GRAVE_ACCENT] = KeyCode::GRAVE;
    translator[GLFW_KEY_EQUAL]        = KeyCode::EQUAL;
    translator[GLFW_KEY_MINUS]        = KeyCode::MINUS;

    // Alphabet
    translator[GLFW_KEY_A] = KeyCode::A;
    translator[GLFW_KEY_B] = KeyCode::B;
    translator[GLFW_KEY_C] = KeyCode::C;
    translator[GLFW_KEY_D] = KeyCode::D;
    translator[GLFW_KEY_E] = KeyCode::E;
    translator[GLFW_KEY_F] = KeyCode::F;
    translator[GLFW_KEY_G] = KeyCode::G;
    translator[GLFW_KEY_H] = KeyCode::H;
    translator[GLFW_KEY_I] = KeyCode::I;
    translator[GLFW_KEY_J] = KeyCode::J;
    translator[GLFW_KEY_K] = KeyCode::K;
    translator[GLFW_KEY_L] = KeyCode::L;
    translator[GLFW_KEY_M] = KeyCode::M;
    translator[GLFW_KEY_N] = KeyCode::N;
    translator[GLFW_KEY_O] = KeyCode::O;
    translator[GLFW_KEY_P] = KeyCode::P;
    translator[GLFW_KEY_Q] = KeyCode::Q;
    translator[GLFW_KEY_R] = KeyCode::R;
    translator[GLFW_KEY_S] = KeyCode::S;
    translator[GLFW_KEY_T] = KeyCode::T;
    translator[GLFW_KEY_U] = KeyCode::U;
    translator[GLFW_KEY_V] = KeyCode::V;
    translator[GLFW_KEY_W] = KeyCode::W;
    translator[GLFW_KEY_X] = KeyCode::X;
    translator[GLFW_KEY_Y] = KeyCode::Y;
    translator[GLFW_KEY_Z] = KeyCode::Z;

    // Punctuation
    translator[GLFW_KEY_LEFT_BRACKET]  = KeyCode::LEFT_BRACKET;
    translator[GLFW_KEY_RIGHT_BRACKET] = KeyCode::RIGHT_BRACKET;
    translator[GLFW_KEY_BACKSLASH]     = KeyCode::BACKSLASH;
    translator[GLFW_KEY_SLASH]         = KeyCode::SLASH;
    translator[GLFW_KEY_COMMA]         = KeyCode::COMMA;
    translator[GLFW_KEY_PERIOD]        = KeyCode::PERIOD;
    translator[GLFW_KEY_SEMICOLON]     = KeyCode::SEMICOLON;
    translator[GLFW_KEY_APOSTROPHE]    = KeyCode::APOSTROPHE;

    // F row
    translator[GLFW_KEY_F1]  = KeyCode::F1;
    translator[GLFW_KEY_F2]  = KeyCode::F2;
    translator[GLFW_KEY_F3]  = KeyCode::F3;
    translator[GLFW_KEY_F4]  = KeyCode::F4;
    translator[GLFW_KEY_F5]  = KeyCode::F5;
    translator[GLFW_KEY_F6]  = KeyCode::F6;
    translator[GLFW_KEY_F7]  = KeyCode::F7;
    translator[GLFW_KEY_F8]  = KeyCode::F8;
    translator[GLFW_KEY_F9]  = KeyCode::F9;
    translator[GLFW_KEY_F10] = KeyCode::F10;
    translator[GLFW_KEY_F11] = KeyCode::F11;
    translator[GLFW_KEY_F12] = KeyCode::F12;
    // Additional F keys
    translator[GLFW_KEY_F13] = KeyCode::F13;
    translator[GLFW_KEY_F14] = KeyCode::F14;
    translator[GLFW_KEY_F15] = KeyCode::F15;
    translator[GLFW_KEY_F16] = KeyCode::F16;
    translator[GLFW_KEY_F17] = KeyCode::F17;
    translator[GLFW_KEY_F18] = KeyCode::F18;
    translator[GLFW_KEY_F19] = KeyCode::F19;
    translator[GLFW_KEY_F20] = KeyCode::F10;

    // Navigation keys
    translator[GLFW_KEY_UP]        = KeyCode::UP;
    translator[GLFW_KEY_DOWN]      = KeyCode::DOWN;
    translator[GLFW_KEY_LEFT]      = KeyCode::LEFT;
    translator[GLFW_KEY_RIGHT]     = KeyCode::RIGHT;
    translator[GLFW_KEY_HOME]      = KeyCode::HOME;
    translator[GLFW_KEY_END]       = KeyCode::END;
    translator[GLFW_KEY_PAGE_UP]   = KeyCode::PAGE_UP;
    translator[GLFW_KEY_PAGE_DOWN] = KeyCode::PAGE_DOWN;

    // Special
    translator[GLFW_KEY_LEFT_ALT]     = KeyCode::LEFT_ALT;
    translator[GLFW_KEY_LEFT_CONTROL] = KeyCode::LEFT_CONTROL;
    translator[GLFW_KEY_LEFT_SHIFT]   = KeyCode::LEFT_SHIFT;
    translator[GLFW_KEY_LEFT_SUPER]   = KeyCode::LEFT_SUPER;

    translator[GLFW_KEY_RIGHT_ALT]     = KeyCode::RIGHT_ALT;
    translator[GLFW_KEY_RIGHT_CONTROL] = KeyCode::RIGHT_CONTROL;
    translator[GLFW_KEY_RIGHT_SHIFT]   = KeyCode::RIGHT_SHIFT;
    translator[GLFW_KEY_RIGHT_SUPER]   = KeyCode::RIGHT_SUPER;

    translator[GLFW_KEY_SPACE]     = KeyCode::SPACE;
    translator[GLFW_KEY_TAB]       = KeyCode::TAB;
    translator[GLFW_KEY_ENTER]     = KeyCode::ENTER;
    translator[GLFW_KEY_ESCAPE]    = KeyCode::ESCAPE;
    translator[GLFW_KEY_BACKSPACE] = KeyCode::BACKSPACE;
    translator[GLFW_KEY_CAPS_LOCK] = KeyCode::CAPITAL;

    translator[GLFW_KEY_INSERT]       = KeyCode::INSERT;
    translator[GLFW_KEY_DELETE]       = KeyCode::DELETE;
    translator[GLFW_KEY_PRINT_SCREEN] = KeyCode::PRINT;
    translator[GLFW_KEY_PAUSE]        = KeyCode::PAUSE;
    translator[GLFW_KEY_SCROLL_LOCK]  = KeyCode::SCROLL_LOCK;

    return translator;
}

} // namespace ENGINE_NAMESPACE

#endif