#include "renderer/lighting/lights.hpp"

namespace ENGINE_NAMESPACE {

Light::Light(const String& name, const glm::vec4& color)
    : name(name), color(color) {}

PointLight::PointLight(
    const String&    name,
    const glm::vec4& color,
    const glm::vec4& position,
    float            constant,
    float            linear,
    float            quadratic,
    float            padding
)
    : Light(name, color), position(position), constant(constant),
      linear(linear), quadratic(quadratic), padding(padding) {}

DirectionalLight::DirectionalLight(
    const String& name, const glm::vec4& color, const glm::vec4& direction
)
    : Light(name, color), direction(direction) {}

} // namespace ENGINE_NAMESPACE