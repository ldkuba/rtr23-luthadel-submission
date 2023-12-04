#include "renderer/lighting/lights.hpp"

namespace ENGINE_NAMESPACE {

Light::Light(const String& name)
    : name(name) {}

PointLight::PointLight(
    const String&    name,
    const PointLightData& data
)
    : Light(name), data(data) {}

DirectionalLight::DirectionalLight(
    const String& name, const DirectionalLightData& data
)
    : Light(name), data(data) {}

} // namespace ENGINE_NAMESPACE