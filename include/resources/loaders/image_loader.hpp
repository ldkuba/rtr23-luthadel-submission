#pragma once

#include "resource_loader.hpp"

class ImageLoader : public ResourceLoader {
  public:
    ImageLoader();
    ~ImageLoader();

    Result<Resource*, RuntimeError> load(const String name);
    void                            unload(Resource* resource);

  private:
};