#pragma once

#include "resource_loader.hpp"

/**
 * @brief Resource loader that handles material config resources.
 */
class MaterialLoader : public ResourceLoader {
  public:
    MaterialLoader();
    ~MaterialLoader();

    Result<Resource*, RuntimeError> load(const String name);
    void                            unload(Resource* resource);
};