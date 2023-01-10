#pragma once

#include "resource_loader.hpp"

/**
 * @brief A resource loader that handles binary files.
 */
class BinaryLoader : public ResourceLoader {
  public:
    BinaryLoader();
    ~BinaryLoader();

    Result<Resource*, RuntimeError> load(const String name);
    void                            unload(Resource* resource);

  private:
};