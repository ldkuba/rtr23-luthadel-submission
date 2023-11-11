#include "systems/input/input_system.hpp"

#include "logger.hpp"

namespace ENGINE_NAMESPACE {

#define INPUT_SYS_LOG "InputSystem :: "

// Constructor & Destructor
InputSystem::InputSystem() {}
InputSystem::~InputSystem() {}

// /////////////////////////// //
// INPUT SYSTEM PUBLIC METHODS //
// /////////////////////////// //

void InputSystem::register_input_source(Platform::Surface* const surface) {
    surface->_input_system = this;

    Logger::trace(INPUT_SYS_LOG, "Input source registered.");
}

Result<Control*, RuntimeError> InputSystem::create_control(
    const String name, const ControlType type
) {
    Logger::trace(INPUT_SYS_LOG, "Creating control \"", name, "\".");

    // If name already exists raise error
    for (const auto* control : _controls) {
        if (control->_name.compare(name) == 0) {
            const auto err_message = String::build(
                "Couldn't create a control with a name \"",
                name,
                "\". This name is already taken. Action failed."
            );
            Logger::error(INPUT_SYS_LOG, err_message);
            return Failure(err_message);
        }
    }

    // If control can be created create it
    Control* new_control = new (MemoryTag::Control) Control(name, type, this);
    _controls.push_back(new_control);

    Logger::trace(INPUT_SYS_LOG, "Control \"", name, "\" created.");
    return new_control;
}
void InputSystem::destroy_control(const String name) {
    // Find control with the given name
    auto it = std::find_if(
        _controls.begin(),
        _controls.end(),
        [name](const Control* const x) { return x->_name.compare(name); }
    );
    auto control = *it;

    // Remove all references to it
    switch (control->_type) {
    case ControlType::Press:
        for (const auto& input : control->_mapped_inputs)
            if (input.type == Control::InputType::Key)
                _on_key_pressed_events[input.code] = nullptr;
        break;
    case ControlType::Release:
        for (const auto& input : control->_mapped_inputs)
            if (input.type == Control::InputType::Key)
                _on_key_released_events[input.code] = nullptr;
        break;
    case ControlType::Hold:
        for (const auto& input : control->_mapped_inputs)
            if (input.type == Control::InputType::Key)
                _on_key_hold_events[input.code] = nullptr;
        break;
    }

    // Delete control object
    _controls.erase(it);
    delete control;

    Logger::trace(INPUT_SYS_LOG, "Control \"", name, "\" destroyed.");
}

void InputSystem::press_key(const KeyCode key) {
    // Get current time just in case
    const auto current_time = Platform::get_absolute_time();

    // Get controls for this key
    auto control_p = _on_key_pressed_events[(InputCode) key];
    auto control_r = _on_key_released_events[(InputCode) key];
    auto control_h = _on_key_hold_events[(InputCode) key];

    // If control for this key exists
    if (control_p) {
        // Calculate timetable
        const auto dt_press      = current_time - control_p->_last_press_t;
        const auto dt_release    = current_time - control_p->_last_release_t;
        control_p->_last_press_t = current_time;

        // Invoke on press event
        control_p->event.invoke(dt_press, dt_release);

        // Register as held
        control_p->_hold_active++;
    }

    // Process other two cases
    if (control_r) {
        control_r->_last_press_t = current_time;
        control_r->_hold_active++;
    }
    if (control_h) {
        control_h->_last_press_t = current_time;
        control_h->_hold_active++;
    }
}
void InputSystem::release_key(const KeyCode key) {
    // Get current time just in case
    const auto current_time = Platform::get_absolute_time();

    // Get controls for this key
    auto control_p = _on_key_pressed_events[(InputCode) key];
    auto control_r = _on_key_released_events[(InputCode) key];
    auto control_h = _on_key_hold_events[(InputCode) key];

    // If control for this key exists
    if (control_r) {
        // Calculate timetable
        const auto dt_press        = current_time - control_r->_last_press_t;
        const auto dt_release      = current_time - control_r->_last_release_t;
        control_r->_last_release_t = current_time;

        // Invoke on release event
        control_r->event.invoke(dt_press, dt_release);

        // Unregister as held
        control_r->_hold_active--;
    }

    // Process other two cases
    if (control_p) {
        control_p->_last_release_t = current_time;
        control_p->_hold_active--;
    }
    if (control_h) {
        control_h->_last_release_t = current_time;
        control_h->_hold_active--;
    }
}
void InputSystem::invoke_held_keys(const float64 delta_time) {
    // Get current time just in case
    const auto current_time = Platform::get_absolute_time();

    // Check all controls and invoke hold ones if the button is held
    for (auto* const control : _controls) {
        if (control->_type == ControlType::Hold && control->_hold_active) {
            control->event(delta_time, current_time - control->_last_press_t);
        }
    }
}

} // namespace ENGINE_NAMESPACE