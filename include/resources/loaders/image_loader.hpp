#pragma once

#include "resource_loader.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Resource loader that handles image files.
 *
 */
class ImageLoader : public ResourceLoader {
  public:
    ImageLoader();
    ~ImageLoader();

    Result<Resource*, RuntimeError> load(const String name);
    void                            unload(Resource* resource);

  private:
    static const std::vector<String> _supported_extensions;
};

} // namespace ENGINE_NAMESPACE