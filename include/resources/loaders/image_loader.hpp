#pragma once

#include "resource_loader.hpp"

class ImageLoader : public ResourceLoader {
  public:
    ImageLoader();
    ~ImageLoader();

    Resource* load(const String name);
    void      unload(Resource* resource);

  private:
};