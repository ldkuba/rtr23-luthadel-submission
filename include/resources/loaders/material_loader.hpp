#ifndef __MATERIAL_LOADER_H__
#define __MATERIAL_LOADER_H__

#pragma once

#include "resource_loader.hpp"

class MaterialLoader : public ResourceLoader {
public:
    MaterialLoader();
    ~MaterialLoader();

    Resource* load(const String name);
    void unload(Resource* resource);

private:

};

#endif // __MATERIAL_LOADER_H__