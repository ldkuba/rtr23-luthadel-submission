#pragma once

#include "renderer/renderer_types.hpp"

namespace ENGINE_NAMESPACE {

class Light {
  public:
    Light(const String& name, const glm::vec3& color);

    String    name;
    glm::vec3 color;
};

/**
 * @brief Omni directional point light
 *
 */
class PointLight : public Light {
  public:
    PointLight(
        const String&    name,
        const glm::vec3& color,
        const glm::vec3& position,
        float            linear    = 1.0,
        float            quadratic = 1.0,
        float            padding   = 1.0
    );

    glm::vec3 position;

    // Fallof factors:
    float linear;
    float quadratic;
    float padding;
};

/**
 * @brief Directional light
 *
 */
class DirectionalLight : public Light {
  public:
    DirectionalLight(
        const String& name, const glm::vec3& color, const glm::vec3& direction
    );

    glm::vec3 direction;
};
} // namespace ENGINE_NAMESPACE