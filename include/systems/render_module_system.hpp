#pragma once

#include "renderer/modules/render_module.hpp"

#include <type_traits>

namespace ENGINE_NAMESPACE {

class RenderModuleSystem {
  public:
    RenderModuleSystem(
        Renderer* const       renderer,
        ShaderSystem* const   shader_system,
        TextureSystem* const  texture_system,
        GeometrySystem* const geometry_system,
        LightSystem* const    light_system
    )
        : _renderer(renderer), _shader_system(shader_system),
          _texture_system(texture_system), _geometry_system(geometry_system),
          _light_system(light_system) {}
    ~RenderModuleSystem() {
        for (const auto& module : _registered_modules)
            del(module);
        _registered_modules.clear();
    }

    template<
        typename T,
        typename = std::enable_if_t<std::is_base_of_v<RenderModule, T>>>
    T* create(const typename T::Config& config) {
        // Create new module
        const auto new_module = new (MemoryTag::RenderModule)
            T(_renderer,
              _shader_system,
              _texture_system,
              _geometry_system,
              _light_system,
              config);
        new_module->initialize(config);

        // Keep track of all created modules
        _registered_modules.push_back(new_module);

        return new_module;
    }

  private:
    Renderer* const       _renderer;
    ShaderSystem* const   _shader_system;
    TextureSystem* const  _texture_system;
    GeometrySystem* const _geometry_system;
    LightSystem* const    _light_system;
    Vector<RenderModule*> _registered_modules {};
};

} // namespace ENGINE_NAMESPACE