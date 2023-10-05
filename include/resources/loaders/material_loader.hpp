#pragma once

#include "resource_loader.hpp"

namespace ENGINE_NAMESPACE {

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

} // namespace ENGINE_NAMESPACE