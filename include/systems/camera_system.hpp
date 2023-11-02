#pragma once

#include "renderer/camera.hpp"
#include "unordered_map.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief System for camera management trough engine.
 */
class CameraSystem {
  public:
    /// @brief Default fallback camera
    Property<Camera*> default_camera {
        GET { return _default_camera; }
    };

    CameraSystem();
    ~CameraSystem();

    /**
     * @brief Acquire camera with a given name. If one doesn't exist new camera
     * is created. Otherwise reference count is increased.
     * @param name Name of requested camera
     * @return Camera* Requested camera object
     */
    Camera* acquire(const String name);

    /**
     * @brief Releases a camera with a given name. Reduces internal reference
     * count. If internal reference count reaches 0 all references to this
     * camera get invalidated.
     * @param name Name of targeted camera for release
     */
    void release(const String name);

  private:
    struct CameraRef {
        Camera* handle;
        uint64  reference_count;
    };

    UnorderedMap<String, CameraRef> _registered_cameras;

    const String _default_camera_name = "default";
    Camera*      _default_camera      = nullptr;

    bool name_is_valid(const String& name);
};

} // namespace ENGINE_NAMESPACE