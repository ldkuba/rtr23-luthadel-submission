#ifndef __BINARY_LOADER_H__
#define __BINARY_LOADER_H__

#pragma once

#include "resource_loader.hpp"

class BinaryLoader : public ResourceLoader {
public:
    BinaryLoader();
    ~BinaryLoader();

    Resource* load(const String name);
    void unload(Resource* resource);

private:
};

#endif // __BINARY_LOADER_H__