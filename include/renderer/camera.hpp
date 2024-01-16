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
            if (_is_dirty) update_view_matrix();
            return _view;
        }
    };
    /// @brief Camera view inverse matrix
    Property<glm::mat4> view_inverse {
        GET {
            if (_is_dirty) update_view_matrix();
            return _view_inverse;
        }
    };

    /// @brief Camera forward vector
    Property<glm::vec3> forward {
        GET { return _forward; }
    };
    /// @brief Camera left vector
    Property<glm::vec3> left {
        GET { return _left; }
    };
    /// @brief Camera up vector
    Property<glm::vec3> up {
        GET { return _up; }
    };

    /// @brief Maximum length a camera name can have
    const static constexpr uint32 max_name_length = 256;

    Camera();
    ~Camera();

    /**
     * @brief Resets camera position and location to default values.
     */
    void reset();

    void move_to(const glm::vec3& position);
    void set_rotation(const glm::quat& rotation);

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
    glm::mat4 _view_inverse;
    bool      _is_dirty = true;

    const glm::vec3 _def_forward { 1, 0, 0 };
    const glm::vec3 _def_left { 0, 1, 0 };
    const glm::vec3 _def_up { 0, 0, 1 };

    glm::vec3 _forward = _def_forward;
    glm::vec3 _left    = _def_left;
    glm::vec3 _up      = _def_up;

    void compute_coord_system();
    void update_view_matrix();
};

} // namespace ENGINE_NAMESPACE