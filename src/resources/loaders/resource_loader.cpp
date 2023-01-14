#include "resources/loaders/resource_loader.hpp"

bool ResourceLoader::can_unload(
    const String resource_type, const Resource* const resource
) {
    if (!resource) {
        Logger::warning(
            RESOURCE_LOG,
            resource_type,
            " unload method called with nullptr. Nothing was done"
        );
        return false;
    }

    if (resource->loader_type().compare_ci(resource_type) != 0) {
        Logger::error(
            RESOURCE_LOG,
            "TYPE loader used for \"",
            resource->loader_type(),
            "\" unloading."
        );
        return false;
    }

    return true;
}