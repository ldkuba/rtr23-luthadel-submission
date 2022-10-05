#ifndef __RESOURCE_SYSTEM_H__
#define __RESOURCE_SYSTEM_H__

#pragma once

#include "resources/loaders/resource_loader.hpp"

#include <unordered_map>

class ResourceSystem {
public:
    static String base_path;

    ResourceSystem();
    ~ResourceSystem();

    // Prevent accidental copying
    ResourceSystem(ResourceSystem const&) = delete;
    ResourceSystem& operator = (ResourceSystem const&) = delete;

    void register_loader(ResourceLoader* const loader);

    Resource* load(const String name, const String type);
    void unload(Resource* resource);

private:
    std::unordered_map<String, ResourceLoader*> _registered_loaders = {};
};

#endif // __RESOURCE_SYSTEM_H__