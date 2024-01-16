#include "renderer/camera.hpp"

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
Camera::Camera() {}
Camera::~Camera() {}

// ///////////////////// //
// CAMERA PUBLIC METHODS //
// ///////////////////// //

void Camera::reset() {
    transform.copy({});
    _forward  = _def_forward;
    _left     = _def_left;
    _up       = _def_up;
    _is_dirty = true;
}

void Camera::move_to(const glm::vec3& position) {
    transform.position = position;
    _is_dirty          = true;
}

void Camera::set_rotation(const glm::quat& rotation) {
    transform.rotation = rotation;
    compute_coord_system();
    _is_dirty = true;
}

void Camera::move_forwards(const float32 amount) {
    transform.translate_by(_forward * amount);
    _is_dirty = true;
}
void Camera::move_backwards(const float32 amount) {
    transform.translate_by(-_forward * amount);
    _is_dirty = true;
}
void Camera::move_up(const float32 amount) {
    transform.translate_by(_up * amount);
    _is_dirty = true;
}
void Camera::move_down(const float32 amount) {
    transform.translate_by(-_up * amount);
    _is_dirty = true;
}
void Camera::move_left(const float32 amount) {
    transform.translate_by(_left * amount);
    _is_dirty = true;
}
void Camera::move_right(const float32 amount) {
    transform.translate_by(-_left * amount);
    _is_dirty = true;
}
void Camera::add_yaw(const float32 amount) {
    transform.rotate_by_deg(_up, amount);
    compute_coord_system();
    _is_dirty = true;
}
void Camera::add_pitch(const float32 amount) {
    // Limit upwards / downwards angle
    const auto angle_between = glm::degrees(glm::acos(glm::dot(_forward, _up)));

    // Compute actual amount
    const auto actual_amount =
        (angle_between + amount < 1.f)     ? 1.f - angle_between
        : (angle_between + amount > 179.f) ? 179.f - angle_between
                                           : amount;

    transform.rotate_by_deg(_left, actual_amount);
    compute_coord_system();
    _is_dirty = true;
}

// ////////////////////// //
// CAMERA PRIVATE METHODS //
// ////////////////////// //

void Camera::compute_coord_system() {
    _forward = glm::mat3_cast(transform.rotation()) * _def_forward;
    _forward = glm::normalize(_forward);
    _left    = glm::cross(_up, _forward);
    _left    = glm::normalize(_left);
}

void Camera::update_view_matrix() {
    _view =
        glm::lookAt(transform.position(), transform.position() + _forward, _up);
    _view_inverse = glm::inverse(_view);
    _is_dirty     = false;
}

} // namespace ENGINE_NAMESPACE