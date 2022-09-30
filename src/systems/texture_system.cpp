#include "systems/texture_system.hpp"

TextureSystem::TextureSystem(Renderer* const renderer) : _renderer(renderer) {
    if (_max_texture_count == 0)
        Logger::fatal("TextureSystem :: _max_texture_count must be greater than 0.");
    create_default_textures();
}
TextureSystem::~TextureSystem() {
    for (auto texture : _registered_textures) {
        _renderer->destroy_texture(texture.second.handle);
        delete texture.second.handle;
    }
    _registered_textures.clear();
    destroy_default_textures();
}

void TextureSystem::create_default_textures() {
    const uint32 texture_dimension = 256;
    const uint32 channels = 4;
    const uint32 pixel_count = texture_dimension * texture_dimension;
    byte pixels[pixel_count * channels] = {};

    for (uint32 row = 0; row < texture_dimension; row++) {
        for (uint32 col = 0; col < texture_dimension; col++) {
            uint32 index = ((row * texture_dimension) + col) * channels;
            pixels[index + 2] = (byte) 255;
            pixels[index + 3] = (byte) 255;
            if ((row / 4) % 2 == (col / 4) % 2) {
                pixels[index + 0] = (byte) 255;
                pixels[index + 1] = (byte) 255;
            }
        }
    }

    _default_texture = new Texture(
        texture_dimension,
        texture_dimension,
        channels,
        _default_texture_name,
        pixels,
        false
    );
    _default_texture->id = 0;
    _renderer->create_texture(_default_texture);
}
void TextureSystem::destroy_default_textures() {
    if (_default_texture) {
        _renderer->destroy_texture(_default_texture);
        delete _default_texture;
    }
}

Texture* TextureSystem::acquire(const String name, const bool auto_release) {
    if (name.length() > Texture::max_name_length) {
        Logger::error("TextureSystem :: Texture couldn't be acquired. ",
            "Maximum name length of a texture is ", Texture::max_name_length,
            " characters but ", name.length(), " character long name was passed. ",
            "Default texture acquired instead.");
        return _default_texture;
    }
    if (name.compare_ci(_default_texture_name) == 0) {
        Logger::warning("TextureSystem :: To acquire the default texture from ",
            "texture system use default_texture property instead.");
        return _default_texture;
    }

    String s = name; s.to_lower();
    auto ref = _registered_textures[s];

    if (ref.handle == nullptr) {
        ref.handle = new Texture(name, "png");
        ref.handle->id = (uint64) ref.handle;
        ref.auto_release = auto_release;
        ref.reference_count = 0;

        // Upload texture to GPU
        _renderer->create_texture(ref.handle);
    }
    ref.reference_count++;

    return ref.handle;
}

void TextureSystem::release(const String name) {
    if (name.compare_ci(_default_texture_name) == 0) {
        Logger::warning("TextureSystem :: Cannot release default texture.");
        return;
    }

    String s = name; s.to_lower();
    auto ref = _registered_textures.find(s);

    if (ref == _registered_textures.end() || ref->second.reference_count == 0) {
        Logger::warning("TextureSystem :: Tried to release a non-existent texture: ", name);
        return;
    }
    ref->second.reference_count--;
    if (ref->second.reference_count == 0 && ref->second.auto_release == true) {
        _renderer->destroy_texture(ref->second.handle);
        delete ref->second.handle;
        _registered_textures.erase(s);
    }
}