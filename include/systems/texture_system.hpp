#pragma once

#include "renderer/renderer.hpp"
#include "resource_system.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Texture system is responsible for management of textures in the
 * engine, including reference counting an auto-unloading.
 */
class TextureSystem {
  public:
    /// @brief Default fallback texture
    Property<Texture*> default_texture {
        GET { return _default_texture; }
    };
    /// @brief Default fallback diffuse texture
    Property<Texture*> default_diffuse_texture {
        GET { return _default_diffuse_texture; }
    };
    /// @brief Default fallback specular texture
    Property<Texture*> default_specular_texture {
        GET { return _default_specular_texture; }
    };
    /// @brief Default fallback normal texture
    Property<Texture*> default_normal_texture {
        GET { return _default_normal_texture; }
    };

    /// @brief Default fallback texture map. Used by shaders when no other map
    /// is specified (Useful during loading)
    Property<Texture::Map*> default_map {
        GET { return _default_map; }
    };

    /**
     * @brief Construct a new Texture System object
     *
     * @param renderer Renderer used by the system
     * @param resource_system Resource system used
     */
    TextureSystem(
        Renderer* const renderer, ResourceSystem* const resource_system
    );
    ~TextureSystem();

    // Prevent accidental copying
    TextureSystem(TextureSystem const&)            = delete;
    TextureSystem& operator=(TextureSystem const&) = delete;

    /// @brief Acquire texture resource from texture system. Texture system will
    /// load the texture from the appropriate location if it's unavailable. If
    /// texture loading fails default is returned instead.
    /// @param name Name of the requested texture
    /// @param auto_release If enabled texture system will automaticaly release
    /// the texture resource from memory if no references to the texture are
    /// detected. Can only be set if the texture resource isn't loaded yet.
    /// @param default_fallback Used in case of acquisition failure. By default
    /// uses texture_system.default_texture.
    /// @returns Texture* Requested texture resource
    Texture* acquire(
        const String   name,
        const bool     auto_release,
        Texture* const default_fallback = nullptr
    );

    /**
     * @brief Attempts to acquire a writable texture with a given name. This
     * wont load resources of the disk. Won't be auto_release.
     *
     * @param name Name of the requested texture
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @param channel_count Number of channels per pixel
     * @param has_transparency Indicates whether the texture will have
     * transparency
     * @return Texture* Requested texture resource
     */
    Texture* acquire_writable(
        const String name,
        const uint32 width,
        const uint32 height,
        const uint8  channel_count,
        const bool   has_transparency
    );

    /**
     * @brief Acquire texture cube resource from texture system. Texture system
     * will load the texture from the appropriate location if it's unavailable.
     * If texture loading fails default is returned instead.
     * @param name Name of the requested texture
     * @param auto_release If enabled texture system will automaticaly release
     * the texture resource from memory if no references to the texture are
     * detected. Can only be set if the texture resource isn't loaded yet.
     * @return Texture*
     */
    Texture* acquire_cube(const String name, const bool auto_release);

    /**
     * @brief Create new texture with provided configuration. If the texture
     * with provided @p config.name already exists it will be overriden.
     * @param config Texture configuration
     * @param data Raw texture image data
     * @param auto_release If enabled texture system will automaticaly release
     * the texture resource from memory if no references to the texture are
     * detected. Can only be set if the texture resource isn't loaded yet.
     * @return Texture*
     */
    Texture* create(
        const Texture::Config& config,
        const byte* const      data,
        const bool             auto_release
    );

    /// @brief Releases texture resource. Texture system will automatically
    /// release this texture from memory if no other references to it are
    /// detected and auto release flag is set to true.
    /// @param name Name of the released texture
    void release(const String name);

  private:
    struct TextureRef {
        Texture* handle;
        uint64   reference_count;
        bool     auto_release;
    };

    Renderer*       _renderer;
    ResourceSystem* _resource_system;

    const String _default_texture_name          = "default";
    const String _default_diffuse_texture_name  = "default_diff";
    const String _default_specular_texture_name = "default_spec";
    const String _default_normal_texture_name   = "default_norm";

    Texture* _default_texture          = nullptr;
    Texture* _default_diffuse_texture  = nullptr;
    Texture* _default_specular_texture = nullptr;
    Texture* _default_normal_texture   = nullptr;

    Texture::Map* _default_map = nullptr;

    UnorderedMap<String, TextureRef> _registered_textures {};

    void create_default_textures();
    void destroy_default_textures();

    Result<void, Texture*> name_is_valid(
        const String& texture_name, Texture* const default_fallback = nullptr
    );
};

} // namespace ENGINE_NAMESPACE