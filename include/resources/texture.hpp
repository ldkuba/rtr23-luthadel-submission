#pragma once

#include "string.hpp"
#include "defines.hpp"
#include "property.hpp"

class Texture {
public:
    Property<int32> width{ Get { return _width; } };
    Property<int32> height{ Get { return _height; } };
    Property<int32> channel_count{ Get { return _channel_count; } };
    Property<uint64> total_size{ Get { return _total_size; } };
    Property<String> name{ Get { return _name; } };
    Property<byte*> pixels{ Get { return _pixels; } };
    Property<bool> has_transparency{ Get { return _has_transparency; } };

    Texture(
        const int32 width,
        const int32 height,
        const int32 channel_count,
        const String name,
        byte* const pixels,
        const bool has_transparency
    );
    Texture(
        const String& name,
        const String& extension
    );
    ~Texture() {}

private:
    int32 _width;
    int32 _height;
    int32 _channel_count;
    String _name;
    byte* _pixels;
    bool _has_transparency;
    uint64 _total_size;
};
