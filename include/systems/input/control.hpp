#pragma once

#include "property.hpp"
#include "event.hpp"
#include "input_codes.hpp"

namespace ENGINE_NAMESPACE {

class InputSystem;

/**
 * @brief Types of control objects. Determines at which point the control will
 * execute its event:
 *  'Press'   executes on key/button press
 *  'Release' executes when key/button is release
 *  'Press'   continuously executes every frame while the key/button is held
 */
enum class ControlType { Press, Release, Hold };

/**
 * @brief Control, represents an object which invokes an event that informs all
 * of its subscribing callbacks of an input change from a input device (ex.
 * keyboard, mouse, controller...). Created and managed by Input system.
 */
class Control {
  public:
    /**
     * @brief Immutable name given to a control
     */
    Property<String> name {
        GET { return _name; }
    };
    /**
     * @brief Event object. Manages callbacks for this control. Callback
     * parameters are dependent on the control type:
     * 'Press'   accept time_since_last_press and time_since_last_release
     * 'Release' accept time_since_last_press and time_since_last_release
     * 'Hold'    accept delta_time and time_since_press
     */
    Event<void(float64, float64)> event {};

    /**
     * @brief Creates a key binding for this control
     *
     * @param key Code for a keyboard key
     */
    void map_key(const KeyCode key);
    /**
     * @brief Removes an existing key binding for this control
     *
     * @param key Code for a keyboard key
     */
    void unmap_key(const KeyCode key);

  private:
    Control(
        const String       name,
        const ControlType  type,
        InputSystem* const input_system
    );
    ~Control();

    friend class InputSystem;

    enum class InputType { Key, Button };
    struct Input {
        InputType type;
        InputCode code;
    };

    // Control description
    const String       _name;
    const ControlType  _type;
    InputSystem* const _input_system;

    // Control state
    Vector<Input> _mapped_inputs {};
    uint32        _hold_active = 0;
    float64       _last_press_t;
    float64       _last_release_t;
};

} // namespace ENGINE_NAMESPACE