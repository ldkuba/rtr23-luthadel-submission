#pragma once

#include "resource.hpp"

class ByteArrayData : public Resource {
  public:
    /// @brief Binary data
    const Vector<byte> data;

    ByteArrayData(const String name, const Vector<byte> data)
        : Resource(name), data(data) {}
    ~ByteArrayData() {}
};

class TextData : public Resource {
  public:
    /// @brief Text data
    const String data;

    TextData(const String name, const String data)
        : Resource(name), data(data) {}
    ~TextData() {}
};