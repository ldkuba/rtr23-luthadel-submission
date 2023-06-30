#include "systems/texture_system.hpp"

#include "resources/image.hpp"

#define TEXTURE_SYS_LOG "TextureSystem :: "

void create_default_textures();
void destroy_default_textures();

// Constructor & Destructor
TextureSystem::TextureSystem(
    Renderer* const renderer, ResourceSystem* const resource_system
)
    : _renderer(renderer), _resource_system(resource_system) {
    Logger::trace(TEXTURE_SYS_LOG, "Creating texture system.");

    if (_max_texture_count == 0)
        Logger::fatal(
            TEXTURE_SYS_LOG, "Const _max_texture_count must be greater than 0."
        );
    create_default_textures();

    Logger::trace(TEXTURE_SYS_LOG, "Texture system created.");
}
TextureSystem::~TextureSystem() {
    for (auto& texture : _registered_textures) {
        _renderer->destroy_texture(texture.second.handle);
        delete texture.second.handle;
    }
    _registered_textures.clear();
    destroy_default_textures();

    Logger::trace(TEXTURE_SYS_LOG, "Texture system destroyed.");
}

// ///////////////////////////// //
// TEXTURE SYSTEM PUBLIC METHODS //
// ///////////////////////////// //

Texture* TextureSystem::acquire(const String name, const bool auto_release) {
    Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" requested.");

    // Check name validity
    if (name.length() > Texture::max_name_length) {
        Logger::error(
            TEXTURE_SYS_LOG,
            "Texture couldn't be acquired. ",
            "Maximum name length of a texture is ",
            Texture::max_name_length,
            " characters but ",
            name.length(),
            " character long name was passed. Default texture acquired instead."
        );
        return _default_texture;
    }
    if (name.compare_ci(_default_texture_name) == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG,
            "To acquire the default texture from texture system use "
            "default_texture property instead."
        );
        return _default_texture;
    }
    if (name.compare_ci(_default_specular_texture_name) == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG,
            "To acquire the default texture from texture system use "
            "default_specular_texture property instead."
        );
        return _default_specular_texture;
    }
    if (name.compare_ci(_default_normal_texture_name) == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG,
            "To acquire the default texture from texture system use "
            "default_normal_texture property instead."
        );
        return _default_normal_texture;
    }

    // Get reference
    String s = name;
    s.to_lower();
    auto ref = _registered_textures.find(s);

    if (ref != _registered_textures.end()) {
        ref->second.reference_count++;

        Logger::trace(TEXTURE_SYS_LOG, "Texture acquired.");
        return ref->second.handle;
    }

    // Texture was just added, load from asset folder
    auto result = _resource_system->load(name, ResourceType::Image);
    if (result.has_error()) {
        Logger::error(
            TEXTURE_SYS_LOG,
            "Texture \"",
            name,
            "\" could be loaded. Returning default_texture."
        );
        return _default_texture;
    }
    auto image = (Image*) result.value();

    auto texture_ref   = TextureRef();
    texture_ref.handle = new (MemoryTag::Texture) Texture(
        name,
        image->width,
        image->height,
        image->channel_count,
        image->has_transparency()
    );
    texture_ref.handle->id      = (uint64) texture_ref.handle;
    texture_ref.auto_release    = auto_release;
    texture_ref.reference_count = 1;

    // Upload texture to GPU
    _renderer->create_texture(texture_ref.handle, image->pixels);

    // Release resources
    _resource_system->unload(image);

    // Cache
    _registered_textures[s] = texture_ref;

    Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" acquired.");
    return texture_ref.handle;
}

void TextureSystem::release(const String name) {
    if (name.compare_ci(_default_texture_name) == 0 ||
        name.compare_ci(_default_specular_texture_name) == 0 ||
        name.compare_ci(_default_normal_texture_name) == 0) {
        return;
    }

    String s = name;
    s.to_lower();
    auto ref = _registered_textures.find(s);

    if (ref == _registered_textures.end() || ref->second.reference_count == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG, "Tried to release a non-existent texture: ", name
        );
        return;
    }
    ref->second.reference_count--;

    // Release resource if it isn't needed
    if (ref->second.reference_count == 0 && ref->second.auto_release == true) {
        _renderer->destroy_texture(ref->second.handle);
        delete ref->second.handle;
        _registered_textures.erase(s);
    }

    Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" released.");
}

// ////////////////////////////// //
// TEXTURE SYSTEM PRIVATE METHODS //
// ////////////////////////////// //

void TextureSystem::create_default_textures() {
    const uint32 texture_dimension = 256;
    const uint32 channels          = 4;
    const uint32 pixel_count       = texture_dimension * texture_dimension;

    byte pixels[pixel_count * channels] = {};

    // Default texture
    for (uint32 row = 0; row < texture_dimension; row++) {
        for (uint32 col = 0; col < texture_dimension; col++) {
            uint32 index      = ((row * texture_dimension) + col) * channels;
            pixels[index + 2] = (byte) 255;
            pixels[index + 3] = (byte) 255;
            if ((row / 4) % 2 == (col / 4) % 2) {
                pixels[index + 0] = (byte) 255;
                pixels[index + 1] = (byte) 255;
            }
        }
    }
    _default_texture = new (MemoryTag::Texture) Texture(
        _default_texture_name,
        texture_dimension,
        texture_dimension,
        channels,
        false
    );
    _default_texture->id = 0;
    _renderer->create_texture(_default_texture, pixels);

    // Diffuse (All white)
    for (auto pixel : pixels)
        pixel = 0xff;
    _default_diffuse_texture = new (MemoryTag::Texture) Texture(
        _default_diffuse_texture_name,
        texture_dimension,
        texture_dimension,
        channels,
        false
    );
    _default_diffuse_texture->id = 1;
    _renderer->create_texture(_default_diffuse_texture, pixels);

    // Specular (full black)
    for (auto pixel : pixels)
        pixel = 0x0;
    _default_specular_texture = new (MemoryTag::Texture) Texture(
        _default_specular_texture_name,
        texture_dimension,
        texture_dimension,
        channels,
        false
    );
    _default_specular_texture->id = 2;
    _renderer->create_texture(_default_specular_texture, pixels);

    // Normal (All up pointing)
    for (uint32 i = 0; i < pixel_count * channels; i++)
        pixels[i] = (((i / 2) % 2) ? 0xff : 0x80);
    _default_normal_texture = new (MemoryTag::Texture) Texture(
        _default_normal_texture_name,
        texture_dimension,
        texture_dimension,
        channels,
        false
    );
    _default_normal_texture->id = 3;
    _renderer->create_texture(_default_normal_texture, pixels);
}
void TextureSystem::destroy_default_textures() {
    if (_default_texture) {
        _renderer->destroy_texture(_default_texture);
        delete _default_texture;
    }
    if (_default_diffuse_texture) {
        _renderer->destroy_texture(_default_diffuse_texture);
        delete _default_diffuse_texture;
    }
    if (_default_specular_texture) {
        _renderer->destroy_texture(_default_specular_texture);
        delete _default_specular_texture;
    }
    if (_default_normal_texture) {
        _renderer->destroy_texture(_default_normal_texture);
        delete _default_normal_texture;
    }
}