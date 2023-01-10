#pragma once

#include "resources/shader.hpp"
#include "texture_system.hpp"

/**
 * @brief Shader system is responsible for management of shader acquisitions and
 * creations. It keeps the reference count, and can be used for auto-updating.
 */
class ShaderSystem {
  public:
    /**
     * @brief Construct a new Shader System object
     *
     * @param renderer Renderer used by the system
     * @param resource_system Resource system used
     * @param texture_system Texture system used
     */
    ShaderSystem(
        Renderer* const       renderer,
        ResourceSystem* const resource_system,
        TextureSystem* const  texture_system
    );
    ~ShaderSystem();

    // Prevent accidental copying
    ShaderSystem(ShaderSystem const&)            = delete;
    ShaderSystem& operator=(ShaderSystem const&) = delete;

    /**
     * @brief Creates a new shader with a given configuration. If shader named
     * given by the configuration is already registered, the previous instance
     * of it will be overriden (Warning about this will then be logged).
     *
     * @param config Shader configuration
     * @returns Plain pointer to the created shader
     * @throws False if creation fails for some reason
     */
    Result<Shader*, bool> create(ShaderConfig config);
    /**
     * @brief Acquire a shader by name. If shader of the given name isn't
     * previously cashed it will be loaded from the shader config asset with the
     * same name.
     *
     * @param name Name of the required shader.
     * @returns Plain pointer to the acquired shader
     * @throws False if acquisition fails for some reason
     */
    Result<Shader*, bool> acquire(const String name);

  private:
    Renderer*       _renderer;
    ResourceSystem* _resource_system;
    TextureSystem*  _texture_system;

    UnorderedMap<String, Shader*> _registered_shaders {};
};