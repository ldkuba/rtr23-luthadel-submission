#include "systems/light_system.hpp"

namespace ENGINE_NAMESPACE {

LightSystem::LightSystem(size_t max_point) {
    _directional_light = nullptr;
    _point_lights.reserve(max_point);

    _max_point_lights = max_point;
}

Outcome LightSystem::add_directional(DirectionalLight* const light) {
    if (_directional_light != nullptr) {
        return Outcome::Failed;
    }

    _directional_light = light;
    return Outcome::Successful;
}

Outcome LightSystem::add_point(PointLight* const light) {
    if (_point_lights.size() >= _max_point_lights) {
        return Outcome::Failed;
    }

    _point_lights.push_back(light);
    return Outcome::Successful;
}

void LightSystem::remove_directional(const DirectionalLight* const light) {
    _directional_light = nullptr;
}

void LightSystem::remove_point(const PointLight* const light) {
    _point_lights.erase(
        std::remove(_point_lights.begin(), _point_lights.end(), light),
        _point_lights.end()
    );
}

const DirectionalLight* const LightSystem::get_directional() {
    return _directional_light;
}

const Vector<PointLight*> LightSystem::get_point() {
    return _point_lights;
}

} // namespace ENGINE_NAMESPACE