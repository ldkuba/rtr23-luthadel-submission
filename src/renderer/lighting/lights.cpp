#include "renderer/lighting/lights.hpp"

namespace ENGINE_NAMESPACE {

Light::Light(const String& name, const glm::vec3& color)
    : name(name), color(color) {}

PointLight::PointLight(
    const String&    name,
    const glm::vec3& color,
    const glm::vec3& position,
    float            linear,
    float            quadratic,
    float            padding
)
    : Light(name, color), position(position), linear(linear),
      quadratic(quadratic), padding(padding) {}

DirectionalLight::DirectionalLight(
    const String& name, const glm::vec3& color, const glm::vec3& direction
)
    : Light(name, color), direction(direction) {}

} // namespace ENGINE_NAMESPACE