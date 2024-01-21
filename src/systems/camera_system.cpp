#include "systems/camera_system.hpp"
#include "logger.hpp"

namespace ENGINE_NAMESPACE {

#define CAMERA_SYS_LOG "CameraSystem :: "

// Constructor & Destructor
CameraSystem::CameraSystem() {
    Logger::trace(CAMERA_SYS_LOG, "Creating camera system.");
    _default_camera = new (MemoryTag::Entity) Camera();
    Logger::trace(CAMERA_SYS_LOG, "Camera system created.");
}
CameraSystem::~CameraSystem() {
    for (const auto& camera : _registered_cameras)
        del(camera.second.handle);
    _registered_cameras.clear();
    del(_default_camera);
    Logger::trace(CAMERA_SYS_LOG, "Camera system destroyed.");
}

// //////////////////////////// //
// CAMERA SYSTEM PUBLIC METHODS //
// //////////////////////////// //

Camera* CameraSystem::acquire(const String name) {
    Logger::trace(CAMERA_SYS_LOG, "Camera `", name, "` requested.");

    // Check name validity
    if (!name_is_valid(name)) return _default_camera;

    // Find requested camera
    const auto ref = _registered_cameras.find(name);

    // Check if found
    if (ref != _registered_cameras.end()) {
        // Camera found, increment reference count
        ref->second.reference_count++;

        // Return results
        Logger::trace(CAMERA_SYS_LOG, "Camera `", name, "` acquired.");
        return ref->second.handle;
    }

    // Camera not found, create a new one
    const auto new_camera = new (MemoryTag::Entity) Camera();

    // Register it
    _registered_cameras[name] = { new_camera, 1 };

    // Return results
    Logger::trace(CAMERA_SYS_LOG, "Camera `", name, "` created and acquired.");
    return new_camera;
}

void CameraSystem::release(const String name) {
    // Make sure its not default
    if (name.compare_ci(_default_camera_name)) return;

    // Find requested camera
    const auto ref = _registered_cameras.find(name);

    // Check if found
    if (ref == _registered_cameras.end()) {
        Logger::warning(
            CAMERA_SYS_LOG, "Tried to release a non-existent camera: ", name
        );
        return;
    }

    // Check if reference count is already 0
    if (ref->second.reference_count == 0) {
        Logger::warning(
            CAMERA_SYS_LOG,
            "Tried to release camera for which all references were already "
            "released. Check usage of camera: ",
            name
        );
        return;
    }

    // Reduce ref count
    ref->second.reference_count--;

    // Release resource if needed
    if (ref->second.reference_count == 0) {
        del(ref->second.handle);
        _registered_cameras.erase(ref->first);
    }

    Logger::trace(CAMERA_SYS_LOG, "Camera `", name, "` released.");
}

// ///////////////////////////// //
// CAMERA SYSTEM PRIVATE METHODS //
// ///////////////////////////// //

bool CameraSystem::name_is_valid(const String& name) {
    if (name.length() == 0) {
        Logger::error(
            CAMERA_SYS_LOG,
            "Camera acquisition failed. Empty camera name not allowed. Default "
            "camera returned instead."
        );
        return false;
    }
    if (name.length() > Camera::max_name_length) {
        Logger::error(
            CAMERA_SYS_LOG,
            "Camera acquisition failed. Maximum name length of a camera is ",
            Camera::max_name_length,
            " characters but ",
            name.length(),
            " character long name was passed. Default camera acquired instead."
        );
        return false;
    }
    if (name.compare_ci(_default_camera_name) == 0) {
        Logger::warning(
            CAMERA_SYS_LOG,
            "To acquire the default camera from camera system use "
            "default_camera property instead."
        );
        return false;
    }
    return true;
}

} // namespace ENGINE_NAMESPACE