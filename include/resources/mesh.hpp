#pragma once

#include "geometry.hpp"
#include "component/transform.hpp"

namespace ENGINE_NAMESPACE {

class Mesh {
  public:
    Transform transform;

    Mesh(
        const Vector<Geometry*>& geometries,
        const Transform&         inital_transform = {}
    );
    Mesh(Geometry* const geometry, const Transform& inital_transform = {});
    ~Mesh();

    void add_geometry_to_render_packet(RenderPacket& render_packet);

  private:
    Vector<Geometry*> _geometries;
};

} // namespace ENGINE_NAMESPACE