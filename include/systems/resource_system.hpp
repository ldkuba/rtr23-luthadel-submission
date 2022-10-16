#pragma once

#include "resources/loaders/resource_loader.hpp"

#include <unordered_map>

class ResourceSystem {
  public:
    /// @brief Base path to assets Folder
    static String base_path;

    ResourceSystem();
    ~ResourceSystem();

    // Prevent accidental copying
    ResourceSystem(ResourceSystem const&)            = delete;
    ResourceSystem& operator=(ResourceSystem const&) = delete;

    /**
     * @brief Register resource loader
     * @param loader Resource loader to register
     */
    void register_loader(ResourceLoader* const loader);

    /**
     * @brief Loads resource off disk
     * @param name Name or relative path to the requested resource
     * @param type Resource type / Name of the resource loader
     * @return Resource*
     */
    Result<Resource*, RuntimeError> load(const String name, const String type);
    /**
     * @brief Unloads loaded resource
     * @param resource Resource to unload
     */
    void                            unload(Resource* resource);

  private:
    std::unordered_map<String, ResourceLoader*> _registered_loaders = {};
};