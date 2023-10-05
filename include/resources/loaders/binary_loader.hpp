#pragma once

#include "resource_loader.hpp"

namespace ENGINE_NAMESPACE {

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

} // namespace ENGINE_NAMESPACE