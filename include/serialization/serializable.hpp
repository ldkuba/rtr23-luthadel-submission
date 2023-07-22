#pragma once

#include "string.hpp"

class Serializer;
class String;
class RuntimeError;

/**
 * @brief An abstract class that provides an interface for objects to be
 * serialized and deserialized.
 * The Serializable class defines two pure virtual functions: serialize() and
 * deserialize(). These functions allow objects to be converted into a
 * serialized format and restored back to their original state.
 */
class Serializable {
  public:
    /**
     * @brief Converts the object into a serialized format using the provided
     * serializer.
     *
     * This pure virtual function must be implemented by derived classes to
     * define the serialization logic for the object. Since it usually only
     * defines order of the used attributes use of `serializable_attributes`
     * macro is recommended.
     *
     * @param serializer A pointer to a Serializer object used for
     * serialization.
     * @return A String object representing the serialized data.
     */
    virtual String serialize(const Serializer* const serializer) const = 0;
    /**
     * @brief Restores the object's original state by deserializing the data
     * using the provided serializer.
     *
     * This pure virtual function must be implemented by derived classes to
     * define the deserialization logic for the object. Since it usually only
     * defines order of the used attributes use of `serializable_attributes`
     * macro is recommended.
     *
     * @param serializer A pointer to a Serializer object used for
     * deserialization.
     * @param data The String object containing the serialized data.
     * @param from_pos The optional starting position for deserialization
     * (default is 0).
     * @return A Result object containing either the position after
     * deserialization or a RuntimeError if deserialization failed.
     */
    virtual Result<uint32, RuntimeError> deserialize(
        const Serializer* const serializer,
        const String&           data,
        const uint32            from_pos = 0
    ) = 0;
};

/**
 * Serializes an object of type T using the provided serializer.
 *
 * @tparam T The type of the object to be serialized.
 * @param obj The object to be serialized.
 * @param serializer The serializer to be used for serialization.
 * @return The serialized object as a String.
 */
template<typename T>
String serialize_object(const T& obj, const Serializer* const serializer) {
    static_assert(
        std::is_same_v<T, void>,
        "Serialization called for non-serializable type."
    );
}

/**
 * Deserializes an object of type T using the provided serializer and data.
 *
 * @tparam T The type of the object to be deserialized.
 * @param obj The object to be deserialized.
 * @param serializer The serializer to be used for deserialization.
 * @param data The data to be deserialized.
 * @param from_pos The starting position in the data for deserialization
 * (default: 0).
 * @return A Result object containing either the position after deserialization
 * or a RuntimeError if deserialization failed.
 */
template<typename T>
Result<uint32, RuntimeError> deserialize_object(
    T&                      obj,
    const Serializer* const serializer,
    const String&           data,
    const uint32            from_pos = 0
) {
    static_assert(
        std::is_same_v<T, void>,
        "Deserialization called for non-serializable type."
    );
}

#define serializable_attributes(attributes...)                                 \
    virtual String serialize(const Serializer* const serializer)               \
        const override {                                                       \
        return serializer->serialize(attributes);                              \
    }                                                                          \
    virtual Result<uint32, RuntimeError> deserialize(                          \
        const Serializer* const serializer,                                    \
        const String&           data,                                          \
        const uint32            from_pos = 0                                   \
    ) override {                                                               \
        return serializer->deserialize(data, from_pos, attributes);            \
    }
