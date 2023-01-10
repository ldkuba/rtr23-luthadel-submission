#pragma once

#include "resource_loader.hpp"

/**
 * @brief Resource loader that handles shader config resources.
 */
class ShaderLoader : public ResourceLoader {
  public:
    ShaderLoader();
    ~ShaderLoader();

    Result<Resource*, RuntimeError> load(const String name);
    void                            unload(Resource* resource);
};
