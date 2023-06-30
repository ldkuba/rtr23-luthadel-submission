#pragma once

#include "geometry.hpp"

class Model : Resource {
  public:
    /// @brief List of geometry configurations
    Property<Vector<GeometryConfig*>> geometry_configs {
        GET { return geometry_configs; }
    };

    Model(const String name, const Vector<GeometryConfig*>& geometry_configs);
    ~Model();

  private:
    Vector<GeometryConfig*> _geometry_configs;
};
