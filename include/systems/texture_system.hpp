#pragma once

#include <map>

#include "resources/texture.hpp"
#include "renderer/renderer.hpp"

class TextureSystem {
private:
    struct TextureRef {
        Texture* handle;
        uint64 reference_count;
        bool auto_release;
    };

public:
    TextureSystem(const Renderer* const renderer);
    ~TextureSystem();

    Texture* acquire(const String name, const bool auto_release);
    void release(const String name);
    Texture* get_default_texture() const;

private:
    const Renderer* _renderer;

    const uint64 _max_texture_count = 1024;
    const String _default_texture_name = "default";

    Texture* _default_texture;
    std::map<const String, TextureRef> _registered_textures;

    void create_default_textures();
    void destroy_default_textures();
};