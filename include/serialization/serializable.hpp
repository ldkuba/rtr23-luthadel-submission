#pragma once

#include "defines.hpp"
#include "result.hpp"

class Serializer;
class String;
class RuntimeError;

class Serializable {
  public:
    virtual String serialize(const Serializer* const serializer) const = 0;
    virtual Result<uint32, RuntimeError> deserialize(
        const String& data, const Serializer* const serializer
    ) = 0;
};

#define serializable_attributes(args...)                                       \
    virtual String serialize(const Serializer* const serializer)               \
        const override {                                                       \
        return serializer->serialize(args);                                    \
    }                                                                          \
    virtual Result<uint32, RuntimeError> deserialize(                          \
        const String& data, const Serializer* const serializer                 \
    ) override {                                                               \
        return serializer->deserialize(data, args);                            \
    }
