#include "component/frustum.hpp"

namespace ENGINE_NAMESPACE {

// -----------------------------------------------------------------------------
// Frustum
// -----------------------------------------------------------------------------

Frustum::Frustum(
    const glm::vec3 position,
    const glm::vec3 forward,
    const glm::vec3 right,
    const glm::vec3 up,
    const float32   aspect_ratio,
    const float32   field_of_view,
    const float32   near,
    const float32   far
) {
    const auto half_v = far * std::tan(field_of_view * 0.5f);
    const auto half_h = half_v * aspect_ratio;

    const auto fn = forward * near;
    const auto ff = forward * far;
    const auto rh = right * half_h;
    const auto uh = up * half_v;

    // Top, Bottom, Right, Left, Far, Near
    _sides[0] = Plane(fn + position, forward);
    _sides[1] = Plane(ff + position, -forward);
    _sides[2] = Plane(position, glm::cross(up, ff + rh));
    _sides[3] = Plane(position, glm::cross(ff - rh, up));
    _sides[4] = Plane(position, glm::cross(right, ff - uh));
    _sides[5] = Plane(position, glm::cross(ff + uh, right));
}
Frustum::~Frustum() {}

// -----------------------------------------------------------------------------
// Plane
// -----------------------------------------------------------------------------

Frustum::Plane::Plane() {}
Frustum::Plane::Plane(const glm::vec3 position, const glm::vec3 normal) {
    equation = glm::vec4(normal, glm::dot(position, normal));
}

float32 Frustum::Plane::signed_distance(const glm::vec3 position) const {
    return glm::dot(equation, glm::vec4(position, -1));
}

template<>
bool Frustum::Plane::contains<3>(const AxisAlignedBBox<3>& aabb) const {
    const auto n = glm::abs(glm::vec3(equation));
    const auto r = glm::dot(0.5f * aabb.get_extent(), n);
    return -r <= signed_distance(aabb.get_center());
}

} // namespace ENGINE_NAMESPACE