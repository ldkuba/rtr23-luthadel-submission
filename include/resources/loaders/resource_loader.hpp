#ifndef __RESOURCE_LOADER_H__
#define __RESOURCE_LOADER_H__

#pragma once

#include "resources/resource.hpp"

#define RESOURCE_LOG "ResourceLoader :: "

struct ResourceType {
    constexpr static const char* const Text = "Text";
    constexpr static const char* const Binary = "Binary";
    constexpr static const char* const Image = "Image";
    constexpr static const char* const Material = "Material";
    constexpr static const char* const StaticMesh = "StaticMesh";
    static bool is_custom(const String type) {
        return (
            (type.compare_ci(Text) != 0) &&
            (type.compare_ci(Binary) != 0) &&
            (type.compare_ci(Image) != 0) &&
            (type.compare_ci(Material) != 0) &&
            (type.compare_ci(StaticMesh) != 0)
            );
    }
};

class ResourceLoader {
public:
    Property<String> type{ Get{ return _type; } };

    ResourceLoader() {}
    ~ResourceLoader() {}

    virtual Resource* load(const String name) { return nullptr; }
    virtual void unload(Resource* resource) {}

protected:
    String _type_path;
    String _type;
};

// Helper define
#define CAN_UNLOAD(TYPE, RESOURCE)                                                  \
    if (!RESOURCE) {                                                                \
        Logger::warning(RESOURCE_LOG, "TYPE unload method called with nullptr. ",   \
            "Nothing was done");                                                    \
        return;                                                                     \
    }                                                                               \
                                                                                    \
    if (RESOURCE->loader_type().compare_ci(ResourceType::TYPE) != 0) {              \
        Logger::error(RESOURCE_LOG, "TYPE loader used for \"",                      \
            RESOURCE->loader_type(), "\" unloading.");                              \
        return;                                                                     \
    }

#endif // __RESOURCE_LOADER_H__