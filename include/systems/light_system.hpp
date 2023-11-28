#pragma once

#include "renderer/lighting/lights.hpp"
#include "outcome.hpp"

#include "vector.hpp"

namespace ENGINE_NAMESPACE {

class LightSystem {
public:
    LightSystem(size_t max_point);

    Outcome add_directional(DirectionalLight* const);
    Outcome add_point(PointLight* const);
    void remove_directional(const DirectionalLight* const);
    void remove_point(const PointLight* const);
    const DirectionalLight* const get_directional();
    DirectionalLightData* get_directional_data();
    const Vector<PointLight*> get_point();
    Vector<PointLightData*> get_point_data();

private:
    DirectionalLight* _directional_light;
    Vector<PointLight*> _point_lights;

    size_t _max_point_lights;
};

} // namespace ENGINE_NAMESPACE