#include "serialization/binary_serializer.hpp"

// /////////////////////////////////// //
// BINARY SERIALIZER PROTECTED METHODS //
// /////////////////////////////////// //

#define SERIALIZE_PRIMITIVE_TYPE(T)                                            \
    void BinarySerializer::serialize_type(String& out_str, const T data)       \
        const {                                                                \
        void* const data_p = (void* const) &data;                              \
        const uint8 size   = sizeof(T);                                        \
        serialize_primitive(out_str, data_p, size);                            \
    }                                                                          \
    Outcome BinarySerializer::deserialize_type(                                \
        const String& in_str, T& data, uint32& position                        \
    ) const {                                                                  \
        byte* const data_p = (byte* const) &data;                              \
        const uint8 size   = sizeof(T);                                        \
        return deserialize_primitive(in_str, position, data_p, size);          \
    }

SERIALIZE_PRIMITIVE_TYPE(bool)
SERIALIZE_PRIMITIVE_TYPE(char)
SERIALIZE_PRIMITIVE_TYPE(int8)
SERIALIZE_PRIMITIVE_TYPE(int16)
SERIALIZE_PRIMITIVE_TYPE(int32)
SERIALIZE_PRIMITIVE_TYPE(int64)
SERIALIZE_PRIMITIVE_TYPE(int128)
SERIALIZE_PRIMITIVE_TYPE(uint8)
SERIALIZE_PRIMITIVE_TYPE(uint16)
SERIALIZE_PRIMITIVE_TYPE(uint32)
SERIALIZE_PRIMITIVE_TYPE(uint64)
SERIALIZE_PRIMITIVE_TYPE(uint128)
SERIALIZE_PRIMITIVE_TYPE(float32)
SERIALIZE_PRIMITIVE_TYPE(float64)

void BinarySerializer::serialize_primitive(
    String& out_str, void* const data, const uint8& size
) const {
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
Outcome BinarySerializer::deserialize_primitive(
    const String& data,
    uint32&       position,
    byte* const   out_data,
    const uint8&  size
) const {
    auto bytes = (byte*) (data.data() + position);
    if (Platform::is_little_endian)
        for (uint32 i = 0; i < size; i++)
            out_data[i] = bytes[size - 1 - i];
    else
        for (uint32 i = 0; i < size; i++)
            out_data[i] = bytes[i];
    position += size;
    return Outcome::Successful;
}

void BinarySerializer::serialize_type(String& out_str, const String& data)
    const {
    out_str += data;
    out_str += '\0';
}
Outcome BinarySerializer::deserialize_type(
    const String& in_str, String& data, uint32& position
) const {
    auto end_i = in_str.find('\0', position);
    if (end_i == std::string::npos) return Outcome::Failed;
    data = in_str.substr(position, end_i - position);
    position += data.size() + 1;
    return Outcome::Successful;
}

void BinarySerializer::serialize_type(String& out_str, const glm::vec1& data)
    const {
    serialize_type(out_str, data.x);
}
void BinarySerializer::serialize_type(String& out_str, const glm::vec2& data)
    const {
    serialize_type(out_str, data.x);
    serialize_type(out_str, data.y);
}
void BinarySerializer::serialize_type(String& out_str, const glm::vec3& data)
    const {
    serialize_type(out_str, data.x);
    serialize_type(out_str, data.y);
    serialize_type(out_str, data.z);
}
void BinarySerializer::serialize_type(String& out_str, const glm::vec4& data)
    const {
    serialize_type(out_str, data.x);
    serialize_type(out_str, data.y);
    serialize_type(out_str, data.z);
    serialize_type(out_str, data.w);
}
void BinarySerializer::serialize_type(String& out_str, const glm::mat2& data)
    const {
    serialize_type(out_str, data[0][0]);
    serialize_type(out_str, data[0][1]);
    serialize_type(out_str, data[1][0]);
    serialize_type(out_str, data[1][1]);
}
void BinarySerializer::serialize_type(String& out_str, const glm::mat3& data)
    const {
    serialize_type(out_str, data[0][0]);
    serialize_type(out_str, data[0][1]);
    serialize_type(out_str, data[0][2]);
    serialize_type(out_str, data[1][0]);
    serialize_type(out_str, data[1][1]);
    serialize_type(out_str, data[1][2]);
    serialize_type(out_str, data[2][0]);
    serialize_type(out_str, data[2][1]);
    serialize_type(out_str, data[2][2]);
}
void BinarySerializer::serialize_type(String& out_str, const glm::mat4& data)
    const {
    serialize_type(out_str, data[0][0]);
    serialize_type(out_str, data[0][1]);
    serialize_type(out_str, data[0][2]);
    serialize_type(out_str, data[0][3]);
    serialize_type(out_str, data[1][0]);
    serialize_type(out_str, data[1][1]);
    serialize_type(out_str, data[1][2]);
    serialize_type(out_str, data[1][3]);
    serialize_type(out_str, data[2][0]);
    serialize_type(out_str, data[2][1]);
    serialize_type(out_str, data[2][2]);
    serialize_type(out_str, data[2][3]);
    serialize_type(out_str, data[3][0]);
    serialize_type(out_str, data[3][1]);
    serialize_type(out_str, data[3][2]);
    serialize_type(out_str, data[3][3]);
}

Outcome BinarySerializer::deserialize_type(
    const String& in_str, glm::vec1& data, uint32& position
) const {
    return deserialize_type(in_str, data.x, position);
}
Outcome BinarySerializer::deserialize_type(
    const String& in_str, glm::vec2& data, uint32& position
) const {
    if (deserialize_type(in_str, data.x, position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data.y, position).failed())
        return Outcome::Failed;
    return Outcome::Successful;
}
Outcome BinarySerializer::deserialize_type(
    const String& in_str, glm::vec3& data, uint32& position
) const {
    if (deserialize_type(in_str, data.x, position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data.y, position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data.z, position).failed())
        return Outcome::Failed;
    return Outcome::Successful;
}
Outcome BinarySerializer::deserialize_type(
    const String& in_str, glm::vec4& data, uint32& position
) const {
    if (deserialize_type(in_str, data.x, position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data.y, position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data.z, position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data.w, position).failed())
        return Outcome::Failed;
    return Outcome::Successful;
}
Outcome BinarySerializer::deserialize_type(
    const String& in_str, glm::mat2& data, uint32& position
) const {
    if (deserialize_type(in_str, data[0][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[0][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][1], position).failed())
        return Outcome::Failed;
    return Outcome::Successful;
}
Outcome BinarySerializer::deserialize_type(
    const String& in_str, glm::mat3& data, uint32& position
) const {
    if (deserialize_type(in_str, data[0][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[0][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[0][2], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][2], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[2][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[2][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[2][2], position).failed())
        return Outcome::Failed;
    return Outcome::Successful;
}
Outcome BinarySerializer::deserialize_type(
    const String& in_str, glm::mat4& data, uint32& position
) const {
    if (deserialize_type(in_str, data[0][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[0][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[0][2], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[0][3], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][2], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[1][3], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[2][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[2][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[2][2], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[2][3], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[3][0], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[3][1], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[3][2], position).failed())
        return Outcome::Failed;
    if (deserialize_type(in_str, data[3][3], position).failed())
        return Outcome::Failed;
    return Outcome::Successful;
}

void BinarySerializer::vector_add_beg(
    String& out_str, const uint64 count, const uint64 type_size
) const {
    serialize_type(out_str, count);
}
Outcome BinarySerializer::vector_remove_beg(
    const String& in_str,
    uint64&       count,
    const uint64  type_size,
    uint32&       position
) const {
    return deserialize_type(in_str, count, position);
}