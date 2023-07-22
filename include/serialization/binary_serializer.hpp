#pragma once

#include "platform/platform.hpp"
#include "serializer.hpp"

/**
 * @brief A class that inherits from the Serializer class and provides
 * functionality for binary serialization and deserialization of various data
 * types.
 *
 */
class BinarySerializer : public Serializer {
  protected:
    virtual void serialize_primitive(
        String& out_str, void* const data, const uint8& size
    ) const;

    virtual bool deserialize_primitive(
        const String& data,
        uint32&       total_read,
        byte* const   out_data,
        const uint8&  size
    ) const;

    // clang-format off
    // === Serialize for types ===
    // Bool
    virtual void serialize_type(String& out_str, const bool data)       const override;
    // Char
    virtual void serialize_type(String& out_str, const char data)       const override;
    // Int
    virtual void serialize_type(String& out_str, const int8 data)       const override;
    virtual void serialize_type(String& out_str, const int16 data)      const override;
    virtual void serialize_type(String& out_str, const int32 data)      const override;
    virtual void serialize_type(String& out_str, const int64 data)      const override;
    virtual void serialize_type(String& out_str, const int128 data)     const override;
    virtual void serialize_type(String& out_str, const uint8 data)      const override;
    virtual void serialize_type(String& out_str, const uint16 data)     const override;
    virtual void serialize_type(String& out_str, const uint32 data)     const override;
    virtual void serialize_type(String& out_str, const uint64 data)     const override;
    virtual void serialize_type(String& out_str, const uint128 data)    const override;
    // Float
    virtual void serialize_type(String& out_str, const float32 data)    const override;
    virtual void serialize_type(String& out_str, const float64 data)    const override;
    // String
    virtual void serialize_type(String& out_str, const String& data)    const override;
    // Math
    virtual void serialize_type(String& out_str, const glm::vec1& data) const override;
    virtual void serialize_type(String& out_str, const glm::vec2& data) const override;
    virtual void serialize_type(String& out_str, const glm::vec3& data) const override;
    virtual void serialize_type(String& out_str, const glm::vec4& data) const override;
    virtual void serialize_type(String& out_str, const glm::mat2& data) const override;
    virtual void serialize_type(String& out_str, const glm::mat3& data) const override;
    virtual void serialize_type(String& out_str, const glm::mat4& data) const override;

    // === Deserialize for types ===
    // Bool
    virtual bool deserialize_type(const String& in_str, bool& data, uint32& position)      const override;
    // Char
    virtual bool deserialize_type(const String& in_str, char& data, uint32& position)      const override;
    // Int
    virtual bool deserialize_type(const String& in_str, int8& data, uint32& position)      const override;
    virtual bool deserialize_type(const String& in_str, int16& data, uint32& position)     const override;
    virtual bool deserialize_type(const String& in_str, int32& data, uint32& position)     const override;
    virtual bool deserialize_type(const String& in_str, int64& data, uint32& position)     const override;
    virtual bool deserialize_type(const String& in_str, int128& data, uint32& position)    const override;
    virtual bool deserialize_type(const String& in_str, uint8& data, uint32& position)     const override;
    virtual bool deserialize_type(const String& in_str, uint16& data, uint32& position)    const override;
    virtual bool deserialize_type(const String& in_str, uint32& data, uint32& position)    const override;
    virtual bool deserialize_type(const String& in_str, uint64& data, uint32& position)    const override;
    virtual bool deserialize_type(const String& in_str, uint128& data, uint32& position)   const override;
    // Float
    virtual bool deserialize_type(const String& in_str, float32& data, uint32& position)   const override;
    virtual bool deserialize_type(const String& in_str, float64& data, uint32& position)   const override;
    // String
    virtual bool deserialize_type(const String& in_str, String& data, uint32& position)    const override;
    // Math
    virtual bool deserialize_type(const String& in_str, glm::vec1& data, uint32& position) const override;
    virtual bool deserialize_type(const String& in_str, glm::vec2& data, uint32& position) const override;
    virtual bool deserialize_type(const String& in_str, glm::vec3& data, uint32& position) const override;
    virtual bool deserialize_type(const String& in_str, glm::vec4& data, uint32& position) const override;
    virtual bool deserialize_type(const String& in_str, glm::mat2& data, uint32& position) const override;
    virtual bool deserialize_type(const String& in_str, glm::mat3& data, uint32& position) const override;
    virtual bool deserialize_type(const String& in_str, glm::mat4& data, uint32& position) const override;
    // clang-format on

    virtual void vector_add_beg(
        String& out_str, const uint64 count, const uint64 type_size
    ) const override;
    virtual bool vector_remove_beg(
        const String& in_str,
        uint64&       count,
        const uint64  type_size,
        uint32&       position
    ) const override;

  private:
    template<typename T>
    void serialize_type(String& out_str, const Vector<T>& data) const {
        serialize_type(out_str, (uint32) data.size());
        for (const auto& data_point : data)
            serialize_one(out_str, data_point);
    }
    template<typename T>
    bool deserialize_type(
        const String& in_str, Vector<T>& data, uint32& position
    ) const {
        // Data size
        uint32 data_count;
        if (!deserialize_type(in_str, data_count, position)) return false;

        // Data
        data.resize(data_count);
        for (auto& data_point : data)
            if (!deserialize_one(in_str, data_point, position)) return false;
        return true;
    }
};