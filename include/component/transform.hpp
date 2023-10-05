#pragma once

#include "math_libs.hpp"
#include "property.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Transform of objects in world. Holds data like position rotation and
 * scale. Can hold pointer to a parent whose transform this transform is
 * relative to.
 */
class Transform {
  public:
    /// @brief In world position
    Property<glm::vec3> position {
        GET { return _position; }
        SET {
            _position = value;
            _is_dirty = true;
        }
    };
    /// @brief In world rotation
    Property<glm::quat> rotation {
        GET { return _rotation; }
        SET {
            _rotation = value;
            _is_dirty = true;
        }
    };
    /// @brief In world scale
    Property<glm::vec3> scale {
        GET { return _scale; }
        SET {
            _scale    = value;
            _is_dirty = true;
        }
    };
    /// @brief Parent transform
    Property<Transform*> parent {
        GET { return _parent; }
        SET {
            _parent   = value;
            _is_dirty = true;
        }
    };

    /**
     * @brief Construct a new Transform object
     *
     * @param _position Transform position (Defaults to zero)
     * @param _rotation Transform rotation (Defaults to identity)
     * @param _scale Transform scale (Defaults to one)
     */
    Transform(
        const glm::vec3 _position = glm::zero<glm::vec3>(),
        const glm::quat _rotation = glm::identity<glm::quat>(),
        const glm::vec3 _scale    = glm::one<glm::vec3>()
    );
    ~Transform();

    /**
     * @brief Translate current transform position
     * @param translation Translation vector
     */
    void translate_by(const glm::vec3 translation);

    /**
     * @brief Rotate current transform
     * @param rotation Required rotation
     */
    void rotate_by(const glm::quat rotation);

    /**
     * @brief Rotate current transform
     *
     * @param axis Rotation axis
     * @param angle Rotation angle
     */
    void rotate_by(const glm::vec3 axis, const float32 angle);

    /**
     * @brief Apply scale to the current transform
     * @param scale Scale amount
     */
    void scale_by(const glm::vec3 scale);

    /// @brief Get local transformation matrix of this transform
    glm::mat4 local();
    /// @brief Get world transformation matrix of this transform
    glm::mat4 world();

  private:
    glm::vec3 _position = {};
    glm::quat _rotation = glm::identity<glm::quat>();
    glm::vec3 _scale    = glm::vec3(1.0f);

    bool _is_dirty = true;

    glm::mat4 _local = glm::identity<glm::mat4>();

    Transform* _parent {};
};

} // namespace ENGINE_NAMESPACE