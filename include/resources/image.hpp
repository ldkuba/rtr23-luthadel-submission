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

    void flip_x() {
        const uint32 row_size = _width * _channel_count;

        byte* top    = _pixels;
        byte* bottom = _pixels + (_height - 1) * row_size;

        while (top < bottom) {
            std::swap_ranges(top, top + row_size, bottom);
            top += row_size;
            bottom -= row_size;
        }
    }
    void flip_y() {
        const uint32 row_size = _width * _channel_count;

        for (uint32 y = 0; y < _height; ++y) {
            byte* row   = _pixels + y * row_size;
            byte* left  = row;
            byte* right = row + row_size - _channel_count;

            while (left < right) {
                for (uint8 i = 0; i < _channel_count; ++i)
                    std::swap(left[i], right[i]);
                left += _channel_count;
                right -= _channel_count;
            }
        }
    }
    void transpose() {
        const uint32 total_size = _width * _height * _channel_count;
        const uint32 pixel_size = _channel_count * sizeof(byte);
        byte*        transposed = new byte[total_size];

        for (uint32 y = 0; y < _height; ++y) {
            for (uint32 x = 0; x < _width; ++x) {
                byte* src  = _pixels + (y * _width + x) * _channel_count;
                byte* dest = transposed + (x * _height + y) * _channel_count;

                std::memcpy(dest, src, pixel_size);
            }
        }

        std::memcpy(_pixels, transposed, total_size);
        delete[] transposed;
    }

  private:
    uint32 _width;
    uint32 _height;
    uint8  _channel_count;
    byte*  _pixels;
};

} // namespace ENGINE_NAMESPACE