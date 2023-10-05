#include "systems/input/control.hpp"

#include "systems/input/input_system.hpp"
#include "logger.hpp"

// Constructor & Destructor
Control::Control(
    const String name, const ControlType type, InputSystem* const input_system
)
    : _name(name), _type(type), _input_system(input_system) {}
Control::~Control() {}

// ////////////////////// //
// CONTROL PUBLIC METHODS //
// ////////////////////// //

void Control::map_key(const KeyCode key) {
    // Add new input mapping
    _mapped_inputs.push_back({ InputType::Key, (InputCode) key });

    // Update appropriate event map
    switch (_type) {
    case ControlType::Press:
        _input_system->_on_key_pressed_events[(InputCode) key] = this;
        break;
    case ControlType::Release:
        _input_system->_on_key_released_events[(InputCode) key] = this;
        break;
    case ControlType::Hold:
        _input_system->_on_key_hold_events[(InputCode) key] = this;
        break;
    }
    return;
}

void Control::unmap_key(const KeyCode key) {
    // Key can only be unmap if it isn't activated
    if (_hold_active) {
        Logger::warning(
            "Control :: Input key cannot be unmapped while control is active. "
            "This is either because key in question is pressed, or some other "
            "mapping of this control is similarly active."
        );
        return;
    }

    // Unmap key from event map
    _input_system->_on_key_pressed_events[(InputCode) key] = nullptr;

    // Unmap key from mapped inputs list
    auto it = std::find_if(
        _mapped_inputs.begin(),
        _mapped_inputs.end(),
        [key](const Input& x) {
            return x.type == InputType::Key && x.code == (InputCode) key;
        }
    );
    _mapped_inputs.erase(it);
}