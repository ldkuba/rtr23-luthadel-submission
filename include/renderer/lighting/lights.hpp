#pragma once

#include "renderer/renderer_types.hpp"

namespace ENGINE_NAMESPACE {

class Light {
  protected:
    Light(const String& name);

  public:
    String name;
};

struct PointLightData {
    glm::vec4 position;
    glm::vec4 color;
    float     constant;
    float     linear;
    float     quadratic;
    float     padding;
};

/**
 * @brief Omni directional point light
 *
 */
class PointLight : public Light {
  public:
    PointLight(const String& name, const PointLightData& data);

    PointLightData data;
};

struct DirectionalLightData {
    glm::vec4 direction;
    glm::vec4 color;
};

/**
 * @brief Directional light
 *
 */
class DirectionalLight : public Light {
  public:
    DirectionalLight(const String& name, const DirectionalLightData& data);

    DirectionalLightData data;
};
} // namespace ENGINE_NAMESPACE