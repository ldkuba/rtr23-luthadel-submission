#pragma once

#include "resource.hpp"

class ByteArrayData : public Resource {
public:
    const std::vector<byte> data;

    ByteArrayData(const String name, const std::vector<byte> data)
        : Resource(name), data(data) {}
    ~ByteArrayData() {}
};


class TextData : public Resource {
public:
    const String data;

    TextData(const String name, const String data)
        : Resource(name), data(data) {}
    ~TextData() {}
};
