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
void Path::add_frame(const MovementFrame& frame) {
    _frames.push_back(frame);
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
            glm::distance(_frames[i].transform.position(), _frames[i - 1].transform.position());
    }

    // Set initial camera position
    _camera->move_to(_frames[0].transform.position());
    _movement_speed = _frames[0].speed;

    // Set initial camera view direction
    _camera->set_rotation(_frames[0].transform.rotation());
    _rotation_speed = _frames[0].rotation_speed;

    // Set initial velocity
    _velocity = _camera->forward() * _movement_speed;

    // Set remaining distance to first frame
    _remaining_distance = glm::distance(
        _frames[0].transform.position(), _frames[1].transform.position()
    );
    _current_segment_distance = _remaining_distance;

    // Set initial frame to 0
    _current_frame = 0;

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
        _frames[_current_frame + 1].transform.position() -
        _frames[_current_frame].transform.position()
    );

    // Interpolate between them
    float32 t = glm::clamp(_movement_speed * delta_time, 0.1, 1.0);
    _velocity = glm::mix(_velocity, direction * _movement_speed, t);

    // Update camera view direction 
    // TODO: make this depend on the rotation speed
    _camera->set_rotation(glm::slerp(
        _camera->transform.rotation(), _frames[_current_frame + 1].transform.rotation(), _rotation_speed * (float32)delta_time * (_current_segment_distance - _remaining_distance) / _current_segment_distance
    ));

    // Update camera position
    glm::vec3 travel_vector = _velocity * (float32) delta_time;
    float32 travelled_distance = glm::length(travel_vector);

    _camera->move_to(
        _camera->transform.position() + travel_vector
    );
    _remaining_distance -= travelled_distance;

    // Update frame
    if (_remaining_distance <= 0.0f) {
        _current_frame++;

        // Compute remaining distance required to reach next frame
        if (_current_frame < _frames.size() - 1) {
            _movement_speed = _frames[_current_frame].speed;
            _rotation_speed = _frames[_current_frame].rotation_speed;
            _remaining_distance = glm::distance(_camera->transform.position(),
                                                _frames[_current_frame + 1].transform.position());
            _current_segment_distance = _remaining_distance;
        }
    }
}

} // namespace ENGINE_NAMESPACE