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

// /////////////////// //
// MESH PUBLIC METHODS //
// /////////////////// //

void Mesh::add_geometry_to_render_packet(RenderPacket& render_packet) {
    const auto model_matrix = transform.world();
    for (const auto geometry : _geometries)
        render_packet.geometry_data.push_back({ geometry, model_matrix });
}

} // namespace ENGINE_NAMESPACE