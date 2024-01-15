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
    
    glm::mat4 get_light_space_matrix(const glm::vec3& camera_pos) const;
    glm::vec4 get_light_camera_position(const glm::vec3& camera_pos) const;

  private:
    // Shadow mapping settings
    struct ShadowmapSettings {
      float32 shadowmap_near_plane;
      float32 shadowmap_far_plane;
      float32 shadowmap_extent;
    } _shadowmap_settings;
};
} // namespace ENGINE_NAMESPACE