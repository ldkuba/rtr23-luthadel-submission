#include "systems/texture_system.hpp"

#include "resources/image.hpp"

namespace ENGINE_NAMESPACE {

#define TEXTURE_SYS_LOG "TextureSystem :: "

void create_default_textures();
void destroy_default_textures();

// Constructor & Destructor
TextureSystem::TextureSystem(
    Renderer* const renderer, ResourceSystem* const resource_system
)
    : _renderer(renderer), _resource_system(resource_system) {
    Logger::trace(TEXTURE_SYS_LOG, "Creating texture system.");

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

Texture* TextureSystem::acquire(
    const String name, const bool auto_release, Texture* const default_fallback
) {
    Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" requested.");

    // Check if name is valid
    const auto name_check_res = name_is_valid(name, default_fallback);
    if (name_check_res.has_error()) return name_check_res.error();

    // If texture already exists, find it
    const auto key = name.lower_c();
    const auto ref = _registered_textures.find(key);

    if (ref != _registered_textures.end()) {
        ref->second.reference_count++;

        Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" acquired.");
        return ref->second.handle;
    }

    // Texture wasn't found, load from asset folder
    auto load_res = _resource_system->load(name, ResourceType::Image);
    if (load_res.has_error()) {
        Logger::error(
            TEXTURE_SYS_LOG,
            "Texture \"",
            name,
            "\" could be loaded. Returning default_texture."
        );
        return (default_fallback == nullptr) ? _default_texture
                                             : default_fallback;
    }
    auto image = (Image*) load_res.value();

    // Create new texture
    const auto texture = _renderer->create_texture(
        { name,
          image->width,
          image->height,
          image->channel_count,
          true, // Mipmaping TODO: conf.
          image->has_transparency() },
        image->pixels
    );
    texture->id = (uint64) texture;

    // Release resources
    _resource_system->unload(image);

    // Create its reference
    _registered_textures[key] = { texture, 1, auto_release };

    Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" acquired.");
    return texture;
}

Texture* TextureSystem::acquire_writable(
    const String name,
    const uint32 width,
    const uint32 height,
    const uint8  channel_count,
    const bool   has_transparency
) {
    Logger::trace(
        TEXTURE_SYS_LOG, "Texture \"", name, "\" (writable) requested."
    );

    // Check if name is valid
    const auto name_check_res = name_is_valid(name);
    if (name_check_res.has_error()) return name_check_res.error();

    // If texture already exists, find it
    const auto key = name.lower_c();
    auto       ref = _registered_textures.find(key);

    if (ref != _registered_textures.end()) {
        ref->second.reference_count++;

        Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" acquired.");
        return ref->second.handle;
    }

    // Texture wasn't found, create new one
    const auto texture = _renderer->create_writable_texture(
        { name, width, height, channel_count, has_transparency, true }
    );
    texture->id = (uint64) texture;

    // Create its reference
    _registered_textures[key] = { texture, 1, false };

    Logger::trace(TEXTURE_SYS_LOG, "Texture \"", name, "\" acquired.");
    return texture;
}

void TextureSystem::release(const String name) {
    if (name.compare_ci(_default_texture_name) == 0 ||
        name.compare_ci(_default_diffuse_texture_name) == 0 ||
        name.compare_ci(_default_specular_texture_name) == 0 ||
        name.compare_ci(_default_normal_texture_name) == 0) {
        return;
    }

    // Find requested texture
    const auto key = name.lower_c();
    const auto ref = _registered_textures.find(key);

    // If not found warn about improper use of this function
    if (ref == _registered_textures.end()) {
        Logger::warning(
            TEXTURE_SYS_LOG, "Tried to release a non-existent texture: ", name
        );
        return;
    }
    // If ref count is 0 this usually means that release is not managed by
    // automatically
    if (ref->second.reference_count == 0) return;

    // Reduce ref count
    ref->second.reference_count--;

    // Release resource if needed
    if (ref->second.reference_count == 0 && ref->second.auto_release == true) {
        _renderer->destroy_texture(ref->second.handle);
        _registered_textures.erase(key);
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

    byte pixels[pixel_count * channels] {};

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
    _default_texture = _renderer->create_texture(
        { _default_texture_name,
          texture_dimension,
          texture_dimension,
          channels,
          true,
          false },
        pixels
    );
    _default_texture->id = 0;

    // Diffuse (All white)
    _default_diffuse_texture = _renderer->create_texture(
        { _default_diffuse_texture_name,
          texture_dimension,
          texture_dimension,
          channels,
          true,
          false },
        pixels
    );
    _default_diffuse_texture->id = 1;

    // Specular (full black)
    for (auto& pixel : pixels)
        pixel = 0x0;
    _default_specular_texture = _renderer->create_texture(
        { _default_specular_texture_name,
          texture_dimension,
          texture_dimension,
          channels,
          true,
          false },
        pixels
    );
    _default_specular_texture->id = 2;

    // Normal (All up pointing)
    for (uint32 i = 0; i < pixel_count * channels; i++)
        pixels[i] = (((i / 2) % 2) ? 0xff : 0x80);
    _default_normal_texture = _renderer->create_texture(
        { _default_normal_texture_name,
          texture_dimension,
          texture_dimension,
          channels,
          true,
          false },
        pixels
    );
    _default_normal_texture->id = 3;
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

Result<void, Texture*> TextureSystem::name_is_valid(
    const String& texture_name, Texture* const default_fallback
) {
    // Check name validity
    if (texture_name.empty()) {
        Logger::error(
            TEXTURE_SYS_LOG,
            "Texture couldn't be acquired. Empty name not allowed. Default "
            "texture acquired instead."
        );
        return Failure(
            (default_fallback == nullptr) ? _default_texture : default_fallback
        );
    }
    if (texture_name.length() > Texture::max_name_length) {
        Logger::error(
            TEXTURE_SYS_LOG,
            "Texture couldn't be acquired. ",
            "Maximum name length of a texture is ",
            Texture::max_name_length,
            " characters but ",
            texture_name.length(),
            " character long name was passed. Default texture acquired instead."
        );
        return Failure(
            (default_fallback == nullptr) ? _default_texture : default_fallback
        );
    }
    if (texture_name.compare_ci(_default_texture_name) == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG,
            "To acquire the default texture from texture system use "
            "default_texture property instead."
        );
        return Failure(_default_texture);
    }
    if (texture_name.compare_ci(_default_diffuse_texture_name) == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG,
            "To acquire the default diffuse texture from texture system use "
            "default_diffuse_texture property instead."
        );
        return Failure(_default_diffuse_texture);
    }
    if (texture_name.compare_ci(_default_specular_texture_name) == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG,
            "To acquire the default specular texture from texture system use "
            "default_specular_texture property instead."
        );
        return Failure(_default_specular_texture);
    }
    if (texture_name.compare_ci(_default_normal_texture_name) == 0) {
        Logger::warning(
            TEXTURE_SYS_LOG,
            "To acquire the default normal texture from texture system use "
            "default_normal_texture property instead."
        );
        return Failure(_default_normal_texture);
    }
    return {};
}

} // namespace ENGINE_NAMESPACE