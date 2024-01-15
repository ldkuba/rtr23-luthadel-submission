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
    enum class Use {
        Unknown,
        MapDiffuse,
        MapSpecular,
        MapNormal,
        MapCube,
        MapPassResult
    };
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
            Texture* texture = nullptr;

            Use    use;
            Filter filter_minify;
            Filter filter_magnify;
            Repeat repeat_u;
            Repeat repeat_v;
            Repeat repeat_w;
        };

        Texture* texture = nullptr;

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

    enum class Format {
        RGBA8Unorm,
        RGBA8Srgb,
        RGBA16Unorm,
        RGBA16Sfloat,
        RGBA32Sfloat,
        BGRA8Unorm,
        BGRA8Srgb,
        D32,
        DS32,
        DS24,
    };

    /**
     * @brief Texture configuration used during initialization
     */
    struct Config {
        const String name;
        const uint32 width;
        const uint32 height;
        const uint32 channel_count;
        const Format format           = Format::RGBA8Unorm;
        const Type   type             = Type::T2D;
        const bool   has_transparency = false;
        const bool   is_mip_mapped    = false;
        const bool   is_writable      = false;
        const bool   is_render_target = false;
        const bool   is_multisampled  = false;
        const bool   is_wrapped       = false;
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

    /// @brief Maximum length a texture name can have
    const static constexpr uint32 max_name_length = 256;

    static bool has_depth_format(Format format);
    inline bool has_depth_format() const {
        return Texture::has_depth_format(_format);
    }

    /**
     * @brief Construct a new Texture object
     * @param config Texture configuration used
     */
    Texture(const Config& config);
    virtual ~Texture() {}

    /// @brief True if texture uses any transparency
    bool has_transparency() const { return _flags & HasTransparency; }
    /// @brief True if this texture can be written to
    bool is_writable() const { return _flags & IsWritable; }
    /// @brief True if this texture was created via wrapping
    bool is_wrapped() const { return _flags & IsWrapped; }
    /// @brief True if image is used as render target
    bool is_render_target() const { return _flags & IsRenderTarget; }
    /// @brief True if image is multisampled
    bool is_multisampled() const { return _flags & IsMultisampled; }

    /// @brief True if any render pass utilizes this texture as an attachment
    bool used_in_render_pass() const { return _flags & UsedInPass; }

    /// @brief Mark texture as one used by a render pass
    virtual void marked_as_used() { _flags |= UsedInPass; }

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

    /**
     * @brief Transition render target into readable format. Only useful if this
     * texture is a render target which we want to sample later on.
     * @param frame_number Frame on which this transition is requested. Only
     * one transition per frame allowed
     */
    virtual Outcome transition_render_target(const uint64 frame_number) = 0;

  protected:
    typedef uint8 TextureFlagType;
    enum TextureFlag : TextureFlagType {
        HasTransparency = 0b000001,
        IsWritable      = 0b000010,
        IsWrapped       = 0b000100,
        IsRenderTarget  = 0b001000,
        IsMultisampled  = 0b010000,
        UsedInPass      = 0b100000
    };

    TextureFlagType _flags;

    String _name = "";
    int32  _width;
    int32  _height;
    int32  _channel_count;
    Format _format;
    uint32 _mip_levels;
    uint64 _total_size;
    Type   _type;

    uint64 _last_transition_frame_number = -1;
};

class PackedTexture : public Texture {
  public:
    PackedTexture(const Config& config, const Vector<Texture*>& textures)
        : Texture(config) {
        _textures.reserve(textures.size());
        for (const auto& texture : textures)
            _textures.push_back(texture);
    }
    ~PackedTexture() {
        for (const auto& texture : _textures)
            del(texture);
        _textures.clear();
    }

    Texture* get_at(uint8 index) const;

    virtual void marked_as_used() override;

    virtual Outcome write(
        const byte* const data, const uint32 size, const uint32 offset
    ) override;
    virtual Outcome resize(const uint32 width, const uint32 height) override;
    virtual Outcome transition_render_target(const uint64 frame_number
    ) override;

  private:
    Vector<Texture*> _textures {};

    uint8 _currently_used_i = 0;
};

} // namespace ENGINE_NAMESPACE