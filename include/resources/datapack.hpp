#pragma once

#include "resource.hpp"

/**
 * @brief Binary data resource. Container for resources loaded form binary.
 */
class ByteArrayData : public Resource {
  public:
    /// @brief Binary data
    const Vector<byte> data;

    ByteArrayData(const String name, const Vector<byte> data)
        : Resource(name), data(data) {}
    ~ByteArrayData() {}
};

/**
 * @brief Plain text resource. Container for data loaded from a text file
 */
class TextData : public Resource {
  public:
    /// @brief Text data
    const String data;

    TextData(const String name, const String data)
        : Resource(name), data(data) {}
    ~TextData() {}
};