#pragma once

#include "platform/platform.hpp"
#include "control.hpp"

/**
 * @brief Input system is responsible for control creation, destruction and
 * invocation, as well as input management.
 *
 */
class InputSystem {
  public:
    /**
     * @brief Construct a new Input System object
     */
    InputSystem();
    ~InputSystem();

    /**
     * @brief Register a source from which inputs can be pooled
     *
     * @param surface A pointer to applications surface which sends input data
     * when in focus.
     */
    void register_input_source(Platform::Surface* const surface);

    /**
     * @brief Create a new control
     *
     * @param name Control name
     * @param type Type of control. Determines control's callback activation
     * point.
     * @return Pointer to a newly created control if successful.
     * @throws RuntimeError If control with this name already exists
     */
    Result<Control*, RuntimeError> create_control(
        const String name, ControlType type
    );
    void destroy_control(const String name);

    /**
     * @brief Signal a key state change to pressed. All press type controls will
     * be signaled. Hold type controls will change their state to active if they
     * are inactive.
     *
     * @param key Code for a keyboard key
     */
    void press_key(const KeyCode key);
    /**
     * @brief Signal a key state change to released. All release type controls
     * will be signaled. Hold type controls wont change their state unless all
     * previously pressed relating keys dont release (This individual release
     * might not be sufficient).
     *
     * @param key Code for a keyboard key
     */
    void release_key(const KeyCode key);
    /**
     * @brief Invokes all controls relating to currently held keys. This method
     * is automaticaly called every frame, and shouldn't be called manually.
     *
     * @param delta_time Time since last frame in seconds
     */
    void invoke_held_keys(const float64 delta_time);

  private:
    friend class Control;

    Vector<Control*> _controls = {};

    Control* _on_key_pressed_events[(InputCode) KeyCode::MAX_KEY_CODE]  = {};
    Control* _on_key_released_events[(InputCode) KeyCode::MAX_KEY_CODE] = {};
    Control* _on_key_hold_events[(InputCode) KeyCode::MAX_KEY_CODE]     = {};
};
