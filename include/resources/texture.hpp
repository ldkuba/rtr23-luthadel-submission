#pragma once

#include "string.hpp"
#include "defines.hpp"
#include "property.hpp"

#include <optional>

struct InternalTextureData {};

enum TextureUse {
    Unknown,
    MapDiffuse
};

class Texture {
public:
    /// @brief Unique texture identifier
    std::optional<uint64> id;
    /// @brief Texture width in pixels
    Property<int32> width{ Get { return _width; } };
    /// @brief Texture height in pixels
    Property<int32> height{ Get { return _height; } };
    /// @brief Number of channels used per pixel
    Property<int32> channel_count{ Get { return _channel_count; } };
    /// @brief Total texture data size in bytes
    Property<uint64> total_size{ Get { return _total_size; } };
    /// @brief Texture name
    Property<String> name{ Get { return _name; } };
    /// @brief Texture data
    Property<byte*> data{ Get { return _data; } };
    /// @brief True if texture uses any transparency
    Property<bool> has_transparency{ Get { return _has_transparency; } };
    /// @brief Pointer to internal texture data managed by the renderer
    Property<InternalTextureData*> internal_data{
        Get { return _internal_data; },
        Set { _internal_data = value; }
    };

    Texture(
        const int32 width,
        const int32 height,
        const int32 channel_count,
        const String name,
        byte* const data,
        const bool has_transparency
    );
    Texture(
        const String& name,
        const String& extension
    );
    ~Texture() {}

    const static uint32 max_name_length = 256;

private:
    int32 _width;
    int32 _height;
    int32 _channel_count;
    String _name;
    byte* _data;
    bool _has_transparency;
    uint64 _total_size;
    InternalTextureData* _internal_data;
};
