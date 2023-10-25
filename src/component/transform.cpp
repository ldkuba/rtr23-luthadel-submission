#include "component/transform.hpp"

namespace ENGINE_NAMESPACE {

Transform::Transform(
    const glm::vec3 position, const glm::quat rotation, const glm::vec3 scale
)
    : _position(position), _rotation(rotation), _scale(scale) {}
Transform::Transform(const Transform& transform)
    : Transform(transform.position, transform.rotation, transform.scale) {}

Transform::~Transform() {}

void Transform::copy(const Transform& transform) {
    _position = transform.position;
    _rotation = transform.rotation;
    _scale    = transform.scale;
}

void Transform::translate_by(const glm::vec3 translation) {
    _position += translation;
    _is_dirty = true;
}
void Transform::rotate_by(const glm::quat rotation) {
    _rotation *= rotation;
    _is_dirty = true;
}
void Transform::rotate_by(const glm::vec3 axis, const float32 angle) {
    _rotation = glm::rotate(_rotation, angle, axis);
    _is_dirty = true;
}
void Transform::rotate_by_deg(const glm::vec3 axis, const float32 angle) {
    _rotation = glm::rotate(_rotation, glm::radians(angle), axis);
    _is_dirty = true;
}
void Transform::scale_by(const glm::vec3 scale) {
    _scale *= scale;
    _is_dirty = true;
}
void Transform::scale_by(const float32 scale) {
    _scale *= scale;
    _is_dirty = true;
}

glm::mat4 Transform::local() {
    if (_is_dirty) {
        _is_dirty = false;

        // Update local model matrix
        _local = glm::mat4_cast(_rotation) *
                 glm::translate(glm::identity<glm::mat4>(), _position);
        _local = glm::scale(_local, _scale);
    }
    return _local;
}
glm::mat4 Transform::world() {
    auto local = this->local();
    if (parent()) return parent()->world() * local;
    return local;
}

} // namespace ENGINE_NAMESPACE