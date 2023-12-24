#pragma once

#include "resource.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Frontend (API agnostic) representation of a texture.
 */
class Texture {
  public:
    /// @brief Collection of various texture types usually used by renderers
    enum class Type { T2D, T2DArray, T3D, TCube };

    /// @brief Collection of texture uses
    enum class Use { Unknown, MapDiffuse, MapSpecular, MapNormal, MapCube };
    /// @brief Collection of supported texture filtering modes
    enum class Filter { NearestNeighbour, BiLinear };
    /// @brief Collection of possible patterns for sampling textures outside
    /// their standard range
    enum class Repeat { Repeat, MirroredRepeat, ClampToEdge, ClampToBorder };

    /**
     * @brief Texture with its relevant properties. Front end representation,
     * renderer agnostic.
     */
    struct Map {
        /**
         * @brief Texture map configuration
         */
        struct Config {
            const Texture* texture = nullptr;

            Use    use;
            Filter filter_minify;
            Filter filter_magnify;
            Repeat repeat_u;
            Repeat repeat_v;
            Repeat repeat_w;
        };

        const Texture* texture = nullptr;

        Use    use;
        Filter filter_minify;
        Filter filter_magnify;
        Repeat repeat_u;
        Repeat repeat_v;
        Repeat repeat_w;

        Map(const Config& config)
            : texture(config.texture), use(config.use),
              filter_minify(config.filter_minify),
              filter_magnify(config.filter_magnify), repeat_u(config.repeat_u),
              repeat_v(config.repeat_v), repeat_w(config.repeat_w) {}
    };

    /**
     * @brief Texture configuration used during initialization
     */
    struct Config {
        const String name             = {};
        const uint32 width            = 0;
        const uint32 height           = 0;
        const uint32 channel_count    = 0;
        const uint32 mip_level_count  = 0;
        const bool   has_transparency = false;
        const bool   is_writable      = false;
        const bool   is_wrapped       = false;
        const Type   type             = Type::T2D;

        Config(
            const String name,
            const uint32 width,
            const uint32 height,
            const uint32 channel_count,
            const bool   mip_mapping,
            const bool   has_transparency,
            const bool   is_writable = false,
            const bool   is_wrapped  = false,
            const Type   type        = Type::T2D
        );
    };

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

    /// @brief True if any render pass utilizes this texture as an attachment
    bool used_by_render_pass = false;

    /// @brief Maximum length a texture name can have
    const static constexpr uint32 max_name_length = 256;

    /**
     * @brief Construct a new Texture object
     * @param config Texture configuration used
     */
    Texture(const Config& config);
    ~Texture() {}

    /// @brief True if texture uses any transparency
    bool has_transparency() const { return _flags & HasTransparency; }
    /// @brief True if this texture can be written to
    bool is_writable() const { return _flags & IsWritable; }
    /// @brief True if this texture was created via wrapping
    bool is_wrapped() const { return _flags & IsWrapped; }

    /**
     * @brief Write raw data into texture. Optimal for writable textures. Works
     * for non-writable textures if whole texture is replaced, and writes are
     * infrequent.
     * @param data Data to be written
     * @param offset Offset from which we want to start writing
     */
    Outcome write(const Vector<byte>& data, const uint32 offset);

    /**
     * @brief Write raw data into texture. Optimal for writable textures. Works
     * for non-writable textures if whole texture is replaced, and writes are
     * infrequent.
     * @param data Data to be written
     * @param size Data size in bytes
     * @param offset Offset from which we want to start writing
     */
    virtual Outcome write(
        const byte* const data, const uint32 size, const uint32 offset
    );

    /**
     * @brief Resizes a given texture. May only be called for writable textures.
     * If texture isn't writable this method will fail.
     * @param texture Texture to be resized
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual Outcome resize(const uint32 width, const uint32 height);

  protected:
    typedef uint8 TextureFlagType;
    enum TextureFlag : TextureFlagType {
        HasTransparency = 0b001,
        IsWritable      = 0b010,
        IsWrapped       = 0b100
    };

    TextureFlagType _flags;

    String _name = "";
    int32  _width;
    int32  _height;
    int32  _channel_count;
    uint32 _mip_levels;
    uint64 _total_size;
    Type   _type;
};

} // namespace ENGINE_NAMESPACE