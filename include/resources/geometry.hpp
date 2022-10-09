#pragma once

#include "material.hpp"

class Geometry : public Resource {
  public:
    std::optional<uint64> internal_id;
    Property<Material*>   material {
        Get { return _material; }
        , Set { _material = value; }
    };

    Geometry(String name);
    ~Geometry();

    const static uint32 max_name_length = 256;

  private:
    Material* _material = nullptr;
};