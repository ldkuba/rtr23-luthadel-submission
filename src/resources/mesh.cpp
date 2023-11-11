#include "resources/mesh.hpp"

namespace ENGINE_NAMESPACE {

Mesh::Mesh(
    const Vector<Geometry*>& geometries, const Transform& inital_transform
) {
    for (const auto& geometry : geometries)
        _geometries.push_back(geometry);
    transform.copy(inital_transform);
}
Mesh::Mesh(Geometry* const geometry, const Transform& inital_transform) {
    _geometries.push_back(geometry);
    transform.copy(inital_transform);
}
Mesh::~Mesh() {}

} // namespace ENGINE_NAMESPACE