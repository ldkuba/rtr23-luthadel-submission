#pragma once

#include "renderer/renderer_types.hpp"

namespace ENGINE_NAMESPACE {

class Light {
  public:
    Light(const String& name, const glm::vec4& color);

    String    name;
    glm::vec4 color;
};

/**
 * @brief Omni directional point light
 *
 */
class PointLight : public Light {
  public:
    PointLight(
        const String&    name,
        const glm::vec4& color,
        const glm::vec4& position,
        float            constant  = 1.0,
        float            linear    = 1.0,
        float            quadratic = 1.0,
        float            padding   = 1.0
    );

    glm::vec4 position;

    // Fallof factors:
    float constant;
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
        const String& name, const glm::vec4& color, const glm::vec4& direction
    );

    glm::vec4 direction;
};
} // namespace ENGINE_NAMESPACE