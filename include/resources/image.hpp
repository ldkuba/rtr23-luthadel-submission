#pragma once

#include "resource.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Image resource.
 *
 */
class Image : public Resource {
  public:
    /// @brief Image width in pixels
    Property<uint32> width {
        GET { return _width; }
    };
    /// @brief Image height in pixels
    Property<uint32> height {
        GET { return _height; }
    };
    /// @brief Image channel count
    Property<uint8> channel_count {
        GET { return _channel_count; }
    };
    /// @brief Raw image pixel data
    Property<const byte*> pixels {
        GET { return _pixels; }
    };

    Image(
        const String name,
        const uint32 width,
        const uint32 height,
        const uint8  channel_count,
        byte* const  pixels
    )
        : Resource(name), _width(width), _height(height),
          _channel_count(channel_count), _pixels(pixels) {}
    ~Image() { delete _pixels; }

    /**
     * @brief Check for image transparency
     *
     * @return true If image has transparency
     * @return false Otherwise
     */
    bool has_transparency() {
        if (_channel_count < 4) return false;

        uint64 total_size = _width * _height * _channel_count;
        for (uint64 i = 3; i < total_size; i += _channel_count)
            if (_pixels[i] < (byte) 255) return true;

        return false;
    }

  private:
    uint32 _width;
    uint32 _height;
    uint8  _channel_count;
    byte*  _pixels;
};

} // namespace ENGINE_NAMESPACE