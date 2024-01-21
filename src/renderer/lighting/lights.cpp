#include <chrono>
#include "renderer/lighting/lights.hpp"
#include "systems/render_view_system.hpp"
#include "renderer/views/render_view_perspective.hpp"
#include "renderer/views/render_view_directional_shadow.hpp"
#include "systems/camera_system.hpp"
#include "renderer/camera.hpp"
#include "resources/mesh.hpp"

namespace ENGINE_NAMESPACE {

Light::Light(const String& name) : name(name) {}

PointLight::PointLight(const String& name, const PointLightData& data)
    : Light(name), data(data),
      _shadowmap_settings({ 0.01f, 100.0f, 90.0f, 8192 }) {}

Vector<RenderViewPerspective*> PointLight::get_render_views() const {
    return _views;
}

void PointLight::enable_shadows(
    RenderViewSystem* render_view_system,
    CameraSystem*     camera_system,
    Vector<Mesh*>&    meshes
) {
    if (_shadows_enabled) return;

    _views.clear();
    create_point_light_views(render_view_system, camera_system, meshes);

    _shadows_enabled = true;
}

void PointLight::create_point_light_views(
    RenderViewSystem* render_view_system,
    CameraSystem*     camera_system,
    Vector<Mesh*>&    meshes
) {
    const glm::vec3 light_pos = glm::vec3(data.position);

    for (size_t i = 0; i < 6; ++i) {
        const String view_name   = name + "_view_" + std::to_string(i);
        const String camera_name = name + "_camera_" + std::to_string(i);

        RenderViewPerspective::Config view_config {
            view_name,
            _shadowmap_settings.shadowmap_size,
            _shadowmap_settings.shadowmap_size,
            RenderView::Type::DefaultPerspective,
            glm::radians(_shadowmap_settings.shadowmap_fov),
            _shadowmap_settings.shadowmap_near_plane,
            _shadowmap_settings.shadowmap_far_plane,
            camera_system->acquire(camera_name)
        };

        _views.push_back(static_cast<RenderViewPerspective*>(
            render_view_system->create(view_config)
                .expect("Point light view creation failed!")
        ));

        _views.at(i)->set_visible_meshes(meshes);
    }
}

void PointLight::set_position(glm::vec3 position) {
    data.position = glm::vec4(position, 1.0);

    // update cameras
    for (auto* view : _views) {
        view->camera()->transform.position = data.position;
    }

    // set flag that light was moved
    recalculate_shadowmap = true;
}

std::array<glm::mat4, 6> PointLight::get_light_space_matrices() const {
    const glm::vec3                light_pos   = glm::vec3(data.position);
    const std::array<glm::mat4, 6> light_views = {
        // Up
        glm::lookAt(
            light_pos,
            light_pos + glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, -1.0f, 0.0f)
        ),
        // Down
        glm::lookAt(
            light_pos,
            light_pos + glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, -1.0f, 0.0f)
        ),
        // Right
        glm::lookAt(
            light_pos,
            light_pos + glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f)
        ),
        // Left
        glm::lookAt(
            light_pos,
            light_pos + glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f)
        ),
        // Forward
        glm::lookAt(
            light_pos,
            light_pos + glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        ),
        // Backward
        glm::lookAt(
            light_pos,
            light_pos + glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        )
    };

    std::array<glm::mat4, 6> light_space_matrices;
    for (size_t i = 0; i < 6; ++i) {
        light_space_matrices[i] = _views.at(i)->proj_matrix() * light_views[i];
    }

    return light_space_matrices;
}

DirectionalLight::DirectionalLight(
    const String& name, const DirectionalLightData& data
)
    : Light(name), data(data), _shadowmap_settings({ 1.0f, 500.0f, 50.0f }) {}

glm::mat4 DirectionalLight::get_light_space_matrix(uint32 rp_index) const {
    auto view = _views.at(rp_index);
    glm::mat4 light_proj = view->proj_matrix();
    glm::mat4 light_view = view->get_view_matrix(data.direction);
    glm::mat4 light_space_matrix = light_proj * light_view;
    return light_space_matrix;
}

Vector<glm::mat4> DirectionalLight::get_light_space_matrices() const {
    Vector<glm::mat4> light_space_matrices;
    for (size_t i = 0; i < _views.size(); ++i) {
        light_space_matrices.push_back(get_light_space_matrix(i));
    }
    return light_space_matrices;
}

glm::vec4 DirectionalLight::get_light_camera_position() const {
    return glm::vec4(
        _views.at(0)->get_light_camera_position(data.direction), 1.0f
    );
}

Vector<RenderViewDirectionalShadow*> DirectionalLight::get_render_views() const {
    return _views;
}

void DirectionalLight::enable_shadows(
    RenderViewSystem* render_view_system,
    CameraSystem*     camera_system,
    Vector<Mesh*>&    meshes,
    uint32            num_shadow_cascades
) {
    if (_shadows_enabled) return;

    _num_shadow_cascades = num_shadow_cascades;

    _views.clear();
    create_cascade_views(render_view_system, camera_system, meshes);

    _shadows_enabled = true;
}

void DirectionalLight::create_cascade_views(
    RenderViewSystem* render_view_system,
    CameraSystem*     camera_system,
    Vector<Mesh*>&    meshes
) {
    for (size_t i = 0; i < _num_shadow_cascades; ++i) {
        const String view_name   = name + "_view_" + std::to_string(i);
        const String camera_name = name + "_camera_" + std::to_string(i);

        RenderViewDirectionalShadow::Config view_config {
            view_name,
            _shadowmap_settings.shadowmap_extent * powf(10.0f, i),
            _shadowmap_settings.shadowmap_extent * powf(10.0f, i),
            RenderView::Type::DirectionalShadow,
            _shadowmap_settings.shadowmap_near_plane,
            _shadowmap_settings.shadowmap_far_plane,
            camera_system->default_camera()
        };

        _views.push_back(static_cast<RenderViewDirectionalShadow*>(
            render_view_system->create(view_config)
                .expect("Point light view creation failed!")
        ));

        _views.at(i)->set_visible_meshes(meshes);
    }
}

} // namespace ENGINE_NAMESPACE
