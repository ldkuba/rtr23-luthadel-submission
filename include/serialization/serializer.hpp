#pragma once

#include "serializable.hpp"
#include "logger.hpp"
#include "math_libs.hpp"

/**
 * @brief The  Serializer  class is an abstract class that provides a blueprint
 * for serialization and deserialization of different data types into a string
 * format. The class is designed to be flexible, allowing for various types of
 * serializers, such as binary, XML, JSON, etc., by implementing the specific
 * serialization and deserialization methods for elemental types or
 * non-elemental types. */
class Serializer {
  public:
    /**
     * @brief Serialize given attribute list as one object.
     *
     * @tparam T Variable length list of attribute types. All attributes listed
     * myst be serializable.
     * @param data Variable length list of attributes as parameters.
     * @return String The serialized attribute list. (as in XML for example.)
     */
    template<typename... T>
    String serialize(const T&... data) const {
        bool   add_sep = true;
        String s {};
        object_add_beg(s);
        (serialize_attribute(s, data, add_sep), ...);
        object_add_end(s);
        return s;
    }

    /**
     * @brief Deserialize attribute list.
     *
     * @tparam T Variable length list of attribute types. All attributes listed
     * myst be serializable.
     * @param data The String to deserialize.
     * @param from_pos Position from which we will start deserializing.
     * @param out_data The loaded data will be stored in this parameters.
     * @return uint32 The number of bytes used for the deserialization which can
     * be used for advancing the position in the string to deserialize next
     * data.
     * @throws RuntimeError If serialization fails.
     */
    template<typename... T>
    Result<uint32, RuntimeError> deserialize(
        const String& data, const uint32 from_pos, T&... out_data
    ) const {
        uint32 position = from_pos;

        // Remove modifiers
        if (!object_remove_beg(data, position)) return _deserialization_failure;
        if (!object_remove_end(data, position)) return _deserialization_failure;

        // Deserialize attributes
        bool successful = true;
        (deserialize_attribute<T>(data, out_data, position, successful), ...);
        if (!successful) return _deserialization_failure;

        return position - from_pos;
    }

  protected:
    // clang-format off
    // === Serialize for types ===
    // Bool
    virtual void serialize_type(String& out_str, const bool data)       const = 0;
    // Char
    virtual void serialize_type(String& out_str, const char data)       const = 0;
    // Int
    virtual void serialize_type(String& out_str, const int8 data)       const = 0;
    virtual void serialize_type(String& out_str, const int16 data)      const = 0;
    virtual void serialize_type(String& out_str, const int32 data)      const = 0;
    virtual void serialize_type(String& out_str, const int64 data)      const = 0;
    virtual void serialize_type(String& out_str, const int128 data)     const = 0;
    virtual void serialize_type(String& out_str, const uint8 data)      const = 0;
    virtual void serialize_type(String& out_str, const uint16 data)     const = 0;
    virtual void serialize_type(String& out_str, const uint32 data)     const = 0;
    virtual void serialize_type(String& out_str, const uint64 data)     const = 0;
    virtual void serialize_type(String& out_str, const uint128 data)    const = 0;
    // Float
    virtual void serialize_type(String& out_str, const float32 data)    const = 0;
    virtual void serialize_type(String& out_str, const float64 data)    const = 0;
    // String
    virtual void serialize_type(String& out_str, const String& data)    const = 0;
    // Math
    virtual void serialize_type(String& out_str, const glm::vec1& data) const = 0;
    virtual void serialize_type(String& out_str, const glm::vec2& data) const = 0;
    virtual void serialize_type(String& out_str, const glm::vec3& data) const = 0;
    virtual void serialize_type(String& out_str, const glm::vec4& data) const = 0;
    virtual void serialize_type(String& out_str, const glm::mat2& data) const = 0;
    virtual void serialize_type(String& out_str, const glm::mat3& data) const = 0;
    virtual void serialize_type(String& out_str, const glm::mat4& data) const = 0;

    // === Deserialize for types ===
    // Bool
    virtual bool deserialize_type(const String& in_str, bool& data, uint32& position)      const = 0;
    // Char
    virtual bool deserialize_type(const String& in_str, char& data, uint32& position)      const = 0;
    // Int
    virtual bool deserialize_type(const String& in_str, int8& data, uint32& position)      const = 0;
    virtual bool deserialize_type(const String& in_str, int16& data, uint32& position)     const = 0;
    virtual bool deserialize_type(const String& in_str, int32& data, uint32& position)     const = 0;
    virtual bool deserialize_type(const String& in_str, int64& data, uint32& position)     const = 0;
    virtual bool deserialize_type(const String& in_str, int128& data, uint32& position)    const = 0;
    virtual bool deserialize_type(const String& in_str, uint8& data, uint32& position)     const = 0;
    virtual bool deserialize_type(const String& in_str, uint16& data, uint32& position)    const = 0;
    virtual bool deserialize_type(const String& in_str, uint32& data, uint32& position)    const = 0;
    virtual bool deserialize_type(const String& in_str, uint64& data, uint32& position)    const = 0;
    virtual bool deserialize_type(const String& in_str, uint128& data, uint32& position)   const = 0;
    // Float
    virtual bool deserialize_type(const String& in_str, float32& data, uint32& position)   const = 0;
    virtual bool deserialize_type(const String& in_str, float64& data, uint32& position)   const = 0;
    // String
    virtual bool deserialize_type(const String& in_str, String& data, uint32& position)    const = 0;
    // Math
    virtual bool deserialize_type(const String& in_str, glm::vec1& data, uint32& position) const = 0;
    virtual bool deserialize_type(const String& in_str, glm::vec2& data, uint32& position) const = 0;
    virtual bool deserialize_type(const String& in_str, glm::vec3& data, uint32& position) const = 0;
    virtual bool deserialize_type(const String& in_str, glm::vec4& data, uint32& position) const = 0;
    virtual bool deserialize_type(const String& in_str, glm::mat2& data, uint32& position) const = 0;
    virtual bool deserialize_type(const String& in_str, glm::mat3& data, uint32& position) const = 0;
    virtual bool deserialize_type(const String& in_str, glm::mat4& data, uint32& position) const = 0;

    // === Padding ===
    // Attribute
    virtual void attribute_add_beg(String& out_string) const {};
    virtual void attribute_add_sep(String& out_string) const {};
    virtual void attribute_add_end(String& out_string) const {};
    virtual bool attribute_remove_beg(const String& in_string, uint32& position) const { return true; };
    virtual bool attribute_remove_sep(const String& in_string, uint32& position) const { return true; };
    virtual bool attribute_remove_end(const String& in_string, uint32& position) const { return true; };
    
    // Whole object
    virtual void object_add_beg(String& out_string) const {};
    virtual void object_add_end(String& out_string) const {};
    virtual bool object_remove_beg(const String& in_string, uint32& position) const { return true; };
    virtual bool object_remove_end(const String& in_string, uint32& position) const { return true; };
    // clang-format on

    // Containers
    virtual void vector_add_beg(
        String& out_str, const uint64 count, const uint64 type_size
    ) const {}
    virtual void vector_add_sep(
        String&      out_str,
        const uint64 count,
        const uint64 type_size,
        const uint64 current
    ) const {}
    virtual void vector_add_end(
        String& out_str, const uint64 count, const uint64 type_size
    ) const {}

    virtual bool vector_remove_beg(
        const String& in_str,
        uint64&       count,
        const uint64  type_size,
        uint32&       position
    ) const {
        return true;
    }
    virtual bool vector_remove_sep(
        const String& in_str,
        uint64&       count,
        const uint64  type_size,
        const uint64  current,
        uint32&       position
    ) const {
        return true;
    }
    virtual bool vector_remove_end(
        const String& in_str,
        uint64&       count,
        const uint64  type_size,
        uint32&       position
    ) const {
        return true;
    }

  private:
    template<typename T>
    void serialize_type(String& out_str, const Vector<T>& data) const {
        const auto count = data.size();
        const auto size  = sizeof(T);
        vector_add_beg(out_str, count, size);
        for (uint64 i = 0; i < count; i++) {
            if (i != 0) vector_add_sep(out_str, count, size, i);
            serialize_one(out_str, data[i]);
        }
        vector_add_end(out_str, count, size);
    }

    template<typename T>
    bool deserialize_type(
        const String& in_str, Vector<T>& data, uint32& position
    ) const {
        uint64     count = 0;
        const auto size  = sizeof(T);

        // Deserialize beginning
        if (!vector_remove_beg(in_str, count, size, position)) return false;
        if (data.size() != count) data.resize(count);

        // Deserialize elements
        for (uint64 i = 0; i < count; i++) {
            if (i != 0 && !vector_remove_sep(in_str, count, size, i, position))
                return false;
            if (!deserialize_one(in_str, data[i], position)) return false;
        }

        // Deserialize end
        if (!vector_remove_end(in_str, count, size, position)) return false;
        if (data.size() != count) data.resize(count);
        return true;
    }

    // Serialize one
    template<typename T>
    void serialize_one(String& out_str, const T& data) const {
        if constexpr (has_serialize_method<Serializer, String&, T>::value)
            serialize_type(out_str, data);
        else if constexpr (std::is_base_of_v<Serializable, T>) {
            auto serializable_data = dynamic_cast<Serializable*>((T*) &data);
            out_str += serializable_data->serialize(this);
        } else out_str += serialize_object(data, this);
    }

    template<typename T>
    bool deserialize_one(const String& data, T& out_data, uint32& position)
        const {
        if constexpr (has_deserialize_method<
                          Serializer,
                          const String&,
                          T&,
                          uint32&>::value)
            return deserialize_type(data, out_data, position);
        else if constexpr (std::is_base_of_v<Serializable, T>) {
            auto serializable_data = dynamic_cast<Serializable*>((T*) &data);
            const auto res =
                serializable_data->deserialize(this, data, position);
            position += res.value_or(0);
            return !res.has_error();
        } else {
            const auto res = deserialize_object(out_data, this, data, position);
            position += res.value_or(0);
            return !res.has_error();
        }
        return false;
    }

    // Helper trait to check if a serializer function exists in a class
    template<typename T, typename... Args>
    struct has_serialize_method {
        template<
            typename C,
            typename = decltype(std::declval<C>()
                                    .serialize_type(std::declval<Args>()...))>
        static constexpr std::true_type test(C*);

        template<typename>
        static constexpr std::false_type test(...);

        using type = decltype(test<std::decay_t<T>>(nullptr));

        static constexpr bool value = type::value;
    };
    template<typename T, typename... Args>
    struct has_deserialize_method {
        template<
            typename C,
            typename = decltype(std::declval<C>()
                                    .deserialize_type(std::declval<Args>()...))>
        static constexpr std::true_type test(C*);

        template<typename>
        static constexpr std::false_type test(...);

        using type = decltype(test<std::decay_t<T>>(nullptr));

        static constexpr bool value = type::value;
    };

    Failure<RuntimeError> _deserialization_failure =
        Failure(RuntimeError("Deserialization failed. Input formatting error.")
        );

    template<typename T>
    void serialize_attribute(
        String& out_str, const T& data, bool& add_separator
    ) const {
        if (add_separator) {
            attribute_add_sep(out_str);
            add_separator = false;
        }
        attribute_add_beg(out_str);
        serialize_one(out_str, data);
        attribute_add_end(out_str);
    }

    template<typename T>
    void deserialize_attribute(
        String const& data, T& out_data, uint32& position, bool& successful
    ) const {
        if (!successful) return;
        successful = false;
        if (!attribute_remove_beg(data, position)) return;
        if (!deserialize_one(data, out_data, position)) return;
        if (!attribute_remove_end(data, position)) return;
        if (!attribute_remove_sep(data, position)) return;
        successful = true;
    }
};