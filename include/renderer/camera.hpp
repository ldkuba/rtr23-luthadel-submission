#pragma once

#include "component/transform.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Camera representation. Ideally created and managed by camera system.
 *
 */
class Camera {
  public:
    /// @brief Camera transform. Only position and rotation have any meaning.
    Transform transform {};

    /// @brief Camera view matrix
    Property<glm::mat4> view {
        GET {
            if (_is_dirty) {
                _view = glm::lookAt(
                    transform.position(), transform.position() + _forward, _up
                );
                _is_dirty = false;
            }
            return _view;
        }
    };

    /// @brief Maximum length a camera name can have
    const static constexpr uint32 max_name_length = 256;

    Camera();
    ~Camera();

    /**
     * @brief Resets camera position and location to default values.
     */
    void reset();

    /**
     * @brief Move camera in forwards direction
     * @param amount Amount of movement in std units
     */
    void move_forwards(const float32 amount);
    /**
     * @brief Move camera in backwards direction
     * @param amount Amount of movement in std units
     */
    void move_backwards(const float32 amount);
    /**
     * @brief Move camera in up direction
     * @param amount Amount of movement in std units
     */
    void move_up(const float32 amount);
    /**
     * @brief Move camera in down direction
     * @param amount Amount of movement in std units
     */
    void move_down(const float32 amount);
    /**
     * @brief Move camera in left direction
     * @param amount Amount of movement in std units
     */
    void move_left(const float32 amount);
    /**
     * @brief Move camera in right direction
     * @param amount Amount of movement in std units
     */
    void move_right(const float32 amount);
    /**
     * @brief Rotate camera around up axis
     * @param amount Rotation amount in degrees
     */
    void add_yaw(const float32 amount);
    /**
     * @brief Rotate camera around right axis
     * @param amount Rotation amount in degrees
     */
    void add_pitch(const float32 amount);

  private:
    glm::mat4 _view;
    bool      _is_dirty = true;

    const glm::vec3 _def_forward { 1, 0, 0 };
    const glm::vec3 _def_left { 0, 1, 0 };
    const glm::vec3 _def_up { 0, 0, 1 };

    glm::vec3 _forward = _def_forward;
    glm::vec3 _left    = _def_left;
    glm::vec3 _up      = _def_up;

    void compute_coord_system();
};

} // namespace ENGINE_NAMESPACE