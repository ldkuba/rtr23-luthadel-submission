#include "systems/resource_system.hpp"

#include "resources/loaders/image_loader.hpp"
#include "resources/loaders/material_loader.hpp"
#include "resources/loaders/binary_loader.hpp"

#define RESOURCE_SYS_LOG "ResourceSystem :: "

// Constructor & Destructor
ResourceSystem::ResourceSystem() {
    // Auto-register known loaders
    ResourceLoader* loader;
    loader = new ImageLoader();
    register_loader(loader);
    loader = new MaterialLoader();
    register_loader(loader);
    loader = new BinaryLoader();
    register_loader(loader);

    Logger::trace(RESOURCE_SYS_LOG, "Resource system initialized.");
}
ResourceSystem::~ResourceSystem() {
    Logger::trace(RESOURCE_SYS_LOG, "Resource system destroyed.");
}

// Static string
String ResourceSystem::base_path = "../assets";

// ////////////////////////////// //
// RESOURCE SYSTEM PUBLIC METHODS //
// ////////////////////////////// //

void ResourceSystem::register_loader(ResourceLoader* const loader) {
    String loader_type = loader->type;

    // Ensure no custom loader with this name exists
    auto loader_it = _registered_loaders.find(loader_type);
    if (loader_it != _registered_loaders.end()) {
        Logger::error(
            RESOURCE_SYS_LOG,
            "Loader of type ",
            loader_type,
            " already exists and wont be registered."
        );
        return;
    }

    // Register custom loader
    _registered_loaders[loader_type] = loader;

    Logger::trace(
        RESOURCE_SYS_LOG,
        "Resource loader \"",
        loader_type,
        "\" was successfully registered."
    );
}

Resource* ResourceSystem::load(const String name, const String type) {
    // Find loader of this type
    auto loader = _registered_loaders.find(type);
    if (loader == _registered_loaders.end()) {
        Logger::error(
            RESOURCE_SYS_LOG,
            "No resource loader of type \"",
            type,
            "\" was found. Resource \"",
            name,
            "\" was unable to be loaded."
        );
        return nullptr;
    }

    // Load
    return loader->second->load(name);
}

void ResourceSystem::unload(Resource* resource) {
    if (resource == nullptr || resource->loader_type().compare("") == 0) return;

    // Find loader of the required type
    auto loader = _registered_loaders.find(resource->loader_type());
    if (loader == _registered_loaders.end()) {
        Logger::error(
            RESOURCE_SYS_LOG,
            "No resource loader of type ",
            resource->loader_type(),
            " was found when attempting to unload resource ",
            resource->name(),
            ". Operation failed."
        );
        return;
    }

    // Unload
    loader->second->unload(resource);
}