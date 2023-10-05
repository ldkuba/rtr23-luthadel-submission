#pragma once

#include "geometry.hpp"
#include "component/transform.hpp"

namespace ENGINE_NAMESPACE {

class Mesh {
  public:
    Mesh();
    ~Mesh();

  private:
    Vector<Geometry*> _geometries;
    Transform         _transform;
};

} // namespace ENGINE_NAMESPACE