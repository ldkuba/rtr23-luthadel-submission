#pragma once

#include "serializable.hpp"
#include "logger.hpp"

class Serializer {
  public:
    template<typename... T>
    String serialize(const T&... data) const {
        String s {};
        (serialize_one(s, data), ...);
        return s;
    }
    template<typename... T>
    Result<uint32, RuntimeError> deserialize(const String& data, T&... out_data)
        const {
        uint32 total_read = 0;
        bool   successful = true;
        (deserialize_one<T>(data, out_data, total_read, successful), ...);
        if (successful) return total_read;
        return Failure(
            RuntimeError("Deserialization failed. Input formatting error.")
        );
    }

  protected:
    virtual void serialize_primitive(
        String& out_str, void* const data, const uint8& size
    ) const = 0;
    virtual bool deserialize_primitive(
        const String& data,
        uint32&       total_read,
        byte* const   out_data,
        const uint8&  size
    ) const = 0;
    virtual void add_to_output(String& out_str, const String& serialized_data)
        const                                            = 0;
    virtual bool subtract_from_input(String& data) const = 0;
    virtual void serialize_string(String& out_str, const String& data)
        const = 0;
    virtual bool deserialize_string(
        const String& data, uint32& total_read, String& out_string
    ) const = 0;

  private:
    template<typename T>
    void serialize_one(String& out_str, const T& data) const {
        if constexpr (std::is_fundamental<T>::value) {
            // Treat fundamental types separately
            T     data_copy = data;
            void* p         = &data_copy;
            uint8 n         = sizeof(T);
            serialize_primitive(out_str, p, n);
        } else {
            // Check if class/struct is serializable. If not bleep about it
            Serializable* serializable_data =
                dynamic_cast<Serializable*>((T*) &data);
            if (!serializable_data)
                Logger::fatal("Serializer :: Type called for serialization "
                              "isn't serializable.");

            // Add serialized content
            add_to_output(out_str, serializable_data->serialize(this));
        }
    }

    template<typename T>
    void deserialize_one(
        const String& data, T& out_data, uint32& total_read, bool& successful
    ) const {
        if (!successful) return;
        if constexpr (std::is_fundamental<T>::value) {
            // Treat fundamental types separately
            byte* p = (byte*) &out_data;
            uint8 n = sizeof(T);
            // Check size
            if (data.size() < total_read + n) {
                successful = false;
                return;
            }
            successful = deserialize_primitive(data, total_read, p, n);
        } else {
            // Check if class/struct is serializable. If not bleep about it
            Serializable* serializable_data =
                dynamic_cast<Serializable*>(&out_data);
            if (!serializable_data)
                Logger::fatal("Serializer :: Type called for deserialization "
                              "isn't serializable.");

            // Add deserialized content
            String rest_of_data = data.substr(total_read);
            successful          = subtract_from_input(rest_of_data);
            if (!successful) return;
            auto result = serializable_data->deserialize(rest_of_data, this);
            if (result.has_error()) successful = false;
            else total_read = result.value();
        }
    }
};

template<>
inline void Serializer::serialize_one<String>(
    String& out_str, const String& data
) const {
    serialize_string(out_str, data);
}
template<>
inline void Serializer::deserialize_one<String>(
    const String& data, String& out_data, uint32& total_read, bool& successful
) const {
    if (!successful) return;
    successful = deserialize_string(data, total_read, out_data);
}