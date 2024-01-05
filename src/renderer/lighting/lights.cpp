#include "renderer/lighting/lights.hpp"
#include <chrono>

namespace ENGINE_NAMESPACE {

Light::Light(const String& name) : name(name) {}

PointLight::PointLight(const String& name, const PointLightData& data)
    : Light(name), data(data) {}

DirectionalLight::DirectionalLight(
    const String& name, const DirectionalLightData& data
)
    : Light(name), data(data), _shadowmap_settings({ 1.0f, 100.0f, 100.0f }) {}

glm::mat4 DirectionalLight::get_light_space_matrix(const glm::vec3& camera_pos
) const {
    const glm::mat4 light_projection = glm::ortho(
        -_shadowmap_settings.shadowmap_extent,
        _shadowmap_settings.shadowmap_extent,
        -_shadowmap_settings.shadowmap_extent,
        _shadowmap_settings.shadowmap_extent,
        _shadowmap_settings.shadowmap_near_plane,
        _shadowmap_settings.shadowmap_far_plane
    );

    glm::vec3 up_vector;
    if(glm::abs(glm::dot(glm::vec3(data.direction), glm::vec3(0.0f, 1.0f, 0.0f))) > 0.99f) {
        up_vector = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        up_vector = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const glm::mat4 light_view = glm::lookAt(
        camera_pos - glm::normalize(glm::vec3(data.direction)) * (_shadowmap_settings.shadowmap_far_plane * 0.5f),
        camera_pos,
        up_vector
    );
    return light_projection * light_view;
}

} // namespace ENGINE_NAMESPACE