#pragma once

#include "resource.hpp"

namespace ENGINE_NAMESPACE {

struct InternalTextureData {};
struct InternalTextureMapData {};

/**
 * @brief Frontend (API agnostic) representation of a texture.
 */
class Texture {
  public:
    /// @brief Unique texture id
    std::optional<uint64> id;
    /// @brief Texture name
    Property<String>      name {
        GET { return _name; }
    };
    /// @brief Texture width in pixels
    Property<int32> width {
        GET { return _width; }
    };
    /// @brief Texture height in pixels
    Property<int32> height {
        GET { return _height; }
    };
    /// @brief Number of channels used per pixel
    Property<int32> channel_count {
        GET { return _channel_count; }
    };
    /// @brief Number of MipMap levels used
    Property<uint32> mip_level_count {
        GET { return _mip_levels; }
    };
    /// @brief Total texture data size in bytes
    Property<uint64> total_size {
        GET { return _total_size; }
    };
    /// @brief True if texture uses any transparency
    Property<bool> has_transparency {
        GET { return _has_transparency; }
    };
    /// @brief True if this texture can be written to
    Property<bool> is_writable {
        GET { return _is_writable; }
    };
    /// @brief Pointer to internal texture data managed by the renderer
    Property<InternalTextureData*> internal_data {
        GET { return _internal_data; }
        SET { _internal_data = value; }
    };

    /**
     * @brief Construct a new Texture object
     *
     * @param name Texture name
     * @param width Texture width in pixels
     * @param height Texture height in pixels
     * @param channel_count Channel count (Typically up to 4). If used,
     * transparency channel is counted.
     * @param has_transparency True if texture uses transparency (Has alpha
     * chanel)
     */
    Texture(
        const String name,
        const int32  width,
        const int32  height,
        const int32  channel_count,
        const bool   has_transparency,
        const bool   is_writable
    );
    ~Texture() {}

    const static uint32 max_name_length = 256;

  private:
    String _name = "";
    int32  _width;
    int32  _height;
    int32  _channel_count;
    int32  _mip_levels;
    uint64 _total_size;
    bool   _has_transparency;
    bool   _is_writable;

    InternalTextureData* _internal_data;
};

/// @brief Collection of texture uses
enum class TextureUse { Unknown, MapDiffuse, MapSpecular, MapNormal };
/// @brief Collection of supported texture filtering modes
enum class TextureFilter { NearestNeighbour, BiLinear };
/// @brief Collection of possible patterns for sampling textures outside their
/// standard range
enum class TextureRepeat { Repeat, MirroredRepeat, ClampToEdge, ClampToBorder };

/**
 * @brief Texture with its relevant properties. Front end representation,
 * renderer agnostic.
 */
struct TextureMap {
    const Texture* texture = nullptr;
    TextureUse     use;
    TextureFilter  filter_minify;
    TextureFilter  filter_magnify;
    TextureRepeat  repeat_u;
    TextureRepeat  repeat_v;
    TextureRepeat  repeat_w;

    InternalTextureMapData* internal_data;
};

} // namespace ENGINE_NAMESPACE