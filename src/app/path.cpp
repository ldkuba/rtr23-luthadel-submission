#include "app/path.hpp"

#include "logger.hpp"

namespace ENGINE_NAMESPACE {

#define PATH_LOG "Path :: "

// Constructor & Destructor
Path::Path() {}
Path::~Path() {}

// /////////////////// //
// PATH PUBLIC METHODS //
// /////////////////// //

void Path::set_camera(Camera* const camera) { _camera = camera; }
void Path::set_movement_speed(float32 speed) { _movement_speed = speed; }
void Path::set_rotation_speed(float32 speed) { _rotation_speed = speed; }
void Path::add_frame(const Transform& transform) {
    _frames.push_back(transform);
}

void Path::start() {
    // Make sure at least 2 frames are set
    if (_frames.size() < 2) {
        Logger::error(PATH_LOG, "Path must have at least 2 frames.");
        return;
    }

    // Compute path length
    _path_length = 0;
    for (uint32 i = 1; i < _frames.size(); i++) {
        _path_length +=
            glm::distance(_frames[i].position(), _frames[i - 1].position());
    }

    // Set initial camera position
    _camera->move_to(_frames[0].position());

    // Set initial camera view direction
    _camera->set_rotation(_frames[0].rotation());

    // Set initial velocity
    _velocity = _camera->forward() * _movement_speed;

    // Set initial frame to 0
    _current_frame = 0;

    // Compute step count required to reach next frame
    _step_count =
        1 + glm::distance(_frames[0].position(), _frames[1].position()) /
                _movement_speed;
    _steps_taken = 0;

    _started = true;
}

void Path::update(const float64 delta_time) {
    if (!_started) return;

    // Check if we already reached last frame
    if (_current_frame >= _frames.size() - 1) {
        _started = false;
        return;
    }

    // Compute movement direction vector
    const auto direction = glm::normalize(
        _frames[_current_frame + 1].position() -
        _frames[_current_frame].position()
    );

    // Interpolate between them
    float32 t = glm::clamp(_movement_speed * delta_time, 0.1, 1.0);
    _velocity = glm::mix(_velocity, direction * _movement_speed, t);

    // Update camera position
    _camera->move_to(
        _camera->transform.position() + _velocity * (float32) delta_time
    );

    // Update camera view direction
    float32 u = glm::clamp(_rotation_speed * delta_time, 0.1, 1.0);
    _camera->set_rotation(glm::slerp(
        _camera->transform.rotation(), _frames[_current_frame + 1].rotation(), u
    ));

    // Update steps
    _steps_taken++;
    if (_steps_taken >= _step_count) {
        _steps_taken = 0;
        _current_frame++;

        // Compute step count required to reach next frame
        if (_current_frame < _frames.size() - 1)
            _step_count = 1 + glm::distance(
                                  _frames[_current_frame].position(),
                                  _frames[_current_frame + 1].position()
                              ) / _movement_speed;
    }
}

} // namespace ENGINE_NAMESPACE