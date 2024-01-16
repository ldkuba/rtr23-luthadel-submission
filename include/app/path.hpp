#pragma once

#include "component/transform.hpp"
#include "renderer/camera.hpp"

namespace ENGINE_NAMESPACE {

struct MovementFrame {
    Transform transform;
    float32 speed;
    float32 rotation_speed;
};

class Path {
  public:
    Path();
    ~Path();

    // Initialization
    void set_camera(Camera* const camera);
    void add_frame(const MovementFrame& frame);

    // Process
    void start();
    void update(const float64 delta_time);

  private:
    Camera* _camera;
    float32 _movement_speed;
    float32 _rotation_speed;
    float32 _path_length = 0;
    bool    _started     = false;

    Vector<MovementFrame> _frames;

    glm::vec3 _velocity;
    glm::vec3 _angular_velocity;

    uint32 _current_frame;
    float32 _remaining_distance;
    float32 _current_segment_distance;
};

} // namespace ENGINE_NAMESPACE