#pragma once

#include "material.hpp"

class Geometry : public Resource {
  public:
    /// @brief Id used by the Renderer
    std::optional<uint64> internal_id;
    /// @brief Material used by the geometry
    Property<Material*>   material {
        GET { return _material; }
        SET { _material = value; }
    };

    Geometry(String name);
    ~Geometry();

    const static uint32 max_name_length = 256;

  private:
    Material* _material = nullptr;
};