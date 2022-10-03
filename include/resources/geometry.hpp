#pragma once

#include "material.hpp"

class Geometry {
public:
    std::optional<uint64> id;
    std::optional<uint64> internal_id;
    Property<String> name{ Get { return _name; } };
    Property<Material*> material{
        Get { return _material; },
        Set { _material = value; }
    };

    Geometry(String name);
    ~Geometry();

    const static uint32 max_name_length = 256;

private:
    String _name;
    Material* _material = nullptr;
};