#pragma once

#include "platform/platform.hpp"
#include "serializer.hpp"

class BinarySerializer : public Serializer {
  protected:
    virtual void serialize_primitive(
        String& out_str, void* const data, const uint8& size
    ) const override {
        byte* bytes = (byte*) data;
        if (Platform::is_little_endian) {
            for (uint32 i = 0; i < size / 2; i++) {
                auto j   = size - 1 - i;
                auto t   = bytes[i];
                bytes[i] = bytes[j];
                bytes[j] = t;
            }
        }
        out_str += std::string(bytes, size);
    }
    virtual bool deserialize_primitive(
        const String& data,
        uint32&       total_read,
        byte* const   out_data,
        const uint8&  size
    ) const override {
        auto bytes = (byte*) (data.data() + total_read);
        if (Platform::is_little_endian)
            for (uint32 i = 0; i < size; i++)
                out_data[i] = bytes[size - 1 - i];
        else
            for (uint32 i = 0; i < size; i++)
                out_data[i] = bytes[i];
        total_read += size;
        return true;
    }
    virtual void add_to_output(String& out_str, const String& serialized_data)
        const override {
        out_str += serialized_data;
    }
    virtual bool subtract_from_input(String& data) const override {
        return true;
    };

    void serialize_string(String& out_str, const String& data) const override {
        out_str += data;
        out_str += '\0';
    }

    bool deserialize_string(
        const String& data, uint32& total_read, String& out_string
    ) const override {
        auto end_i = data.find('\0', total_read);
        if (end_i == std::string::npos) return false;
        out_string = data.substr(total_read, end_i - total_read);
        total_read += out_string.size() + 1;
        return true;
    }
};