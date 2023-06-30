#pragma once

#include "geometry.hpp"
#include "component/transform.hpp"

class Mesh {
  public:
    Mesh();
    ~Mesh();

  private:
    Vector<Geometry*> _geometries;
    Transform         _transform;
};
