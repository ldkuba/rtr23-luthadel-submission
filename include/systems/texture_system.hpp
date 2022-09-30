#pragma once

#include <map>

#include "resources/texture.hpp"
#include "renderer/renderer.hpp"

class TextureSystem {
public:
    /// @brief pinter to the default texture
    Property<Texture*> default_texture{ Get { return _default_texture; } };

    TextureSystem(Renderer* const renderer);
    ~TextureSystem();

    /// @brief Acquire texture resource from texture system. Texture system will load 
    /// the texture from the appropriate location if it's unavailable.
    /// @param name Name of the requested texture
    /// @param auto_release If enabled texture system will automaticaly release 
    /// the texture resource from memory if no references to the texture are detected.
    /// Can only be set if the texture resource isn't loaded yet.
    /// @returns Requested texture resource
    Texture* acquire(const String name, const bool auto_release);
    /// @brief Releases texture resource. Texture system will automatically release 
    /// this texture from memory if no other references to it are detected and auto 
    /// release flag is set to true.
    /// @param name Name of the released texture
    void release(const String name);

private:
    struct TextureRef {
        Texture* handle;
        uint64 reference_count;
        bool auto_release;
    };

    Renderer* _renderer;

    const uint64 _max_texture_count = 1024;
    const String _default_texture_name = "default";

    Texture* _default_texture = nullptr;
    std::map<const String, TextureRef> _registered_textures;

    void create_default_textures();
    void destroy_default_textures();
};