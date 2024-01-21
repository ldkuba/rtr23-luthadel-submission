#pragma once

#include "renderer/renderer_types.hpp"
#include "containers/vector.hpp"

namespace ENGINE_NAMESPACE {

class Light {
  protected:
    Light(const String& name);

    bool _shadows_enabled = false;

  public:
    String name;

    bool get_shadows_enabled() const { return _shadows_enabled; }
};

struct PointLightData {
  private:
    // Use setter in PointLight class
    glm::vec4 position;

  public:
    glm::vec4 color;
    float     constant;
    float     linear;
    float     quadratic;
    float     padding;

    PointLightData(
        glm::vec4 position,
        glm::vec4 color,
        float     constant,
        float     linear,
        float     quadratic
    )
        : position(position), color(color), constant(constant), linear(linear),
          quadratic(quadratic) {}

    friend class PointLight;
};

class RenderViewSystem;
class RenderViewPerspective;
class RenderViewDirectionalShadow;
class CameraSystem;
class Camera;
class Mesh;

/**
 * @brief Omni directional point light
 *
 */
class PointLight : public Light {
  public:
    PointLight(const String& name, const PointLightData& data);

    std::array<glm::mat4, 6> get_light_space_matrices() const;

    void enable_shadows(
        RenderViewSystem* render_view_system,
        CameraSystem*     camera_system,
        Vector<Mesh*>&    meshes
    );

    Vector<RenderViewPerspective*> get_render_views() const;

    PointLightData data;
    void           set_position(glm::vec3 position);

    bool recalculate_shadowmap = true;

  private:
    // Shadow mapping settings
    struct ShadowmapSettings {
        float32 shadowmap_near_plane;
        float32 shadowmap_far_plane;
        float32 shadowmap_fov;
        float32 shadowmap_size; // Size of full shadowmap texture
    } _shadowmap_settings;

    Vector<RenderViewPerspective*> _views;

    void create_point_light_views(
        RenderViewSystem* render_view_system,
        CameraSystem*     camera_system,
        Vector<Mesh*>&    meshes
    );
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

    glm::mat4 get_light_space_matrix(uint32 rp_index) const;
    Vector<glm::mat4> get_light_space_matrices() const;
    glm::vec4 get_light_camera_position() const;
    Vector<RenderViewDirectionalShadow*> get_render_views() const;

    void enable_shadows(
        RenderViewSystem* render_view_system,
        CameraSystem*     camera_system,
        Vector<Mesh*>&    meshes,
        uint32            num_shadow_cascades
    );

  private:
    // Shadow mapping settings
    struct ShadowmapSettings {
        float32 shadowmap_near_plane;
        float32 shadowmap_far_plane;
        float32 shadowmap_extent;
    } _shadowmap_settings;

    Vector<RenderViewDirectionalShadow*> _views;
    uint32                               _num_shadow_cascades;

    void create_cascade_views(
        RenderViewSystem* render_view_system,
        CameraSystem*     camera_system,
        Vector<Mesh*>&    meshes
    );
};
} // namespace ENGINE_NAMESPACE