#pragma once

#include "component/transform.hpp"
#include "renderer/camera.hpp"

namespace ENGINE_NAMESPACE {

class Path {
  public:
    Path();
    ~Path();

    // Initialization
    void set_camera(Camera* const camera);
    void set_movement_speed(float32 speed);
    void set_rotation_speed(float32 speed);
    void add_frame(const Transform& transform);

    // Process
    void start();
    void update(const float64 delta_time);

  private:
    Camera* _camera;
    float32 _movement_speed;
    float32 _rotation_speed;
    float32 _path_length = 0;
    bool    _started     = false;

    Vector<Transform> _frames;

    glm::vec3 _velocity;
    glm::vec3 _angular_velocity;

    uint32 _current_frame;
    uint32 _step_count;
    uint32 _steps_taken;
};

} // namespace ENGINE_NAMESPACE