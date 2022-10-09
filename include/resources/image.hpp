#pragma once

#include "resource.hpp"

class Image : public Resource {
  public:
    Property<uint32> width {
        Get { return _width; }
    };
    Property<uint32> height {
        Get { return _height; }
    };
    Property<uint8> channel_count {
        Get { return _channel_count; }
    };
    Property<const byte*> pixels {
        Get { return _pixels; }
    };

    Image(
        const String      name,
        const uint32      width,
        const uint32      height,
        const uint8       channel_count,
        const byte* const pixels
    )
        : Resource(name), _width(width), _height(height),
          _channel_count(channel_count), _pixels(pixels) {}
    ~Image() { delete _pixels; }

    bool has_transparency() {
        if (_channel_count < 4) return false;

        uint64 total_size = _width * _height * _channel_count;
        for (uint64 i = 3; i < total_size; i += _channel_count)
            if (_pixels[i] < (byte) 255) return true;

        return false;
    }

  private:
    uint32      _width;
    uint32      _height;
    uint8       _channel_count;
    const byte* _pixels;
};