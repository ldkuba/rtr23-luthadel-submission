#pragma once

#include "resource_loader.hpp"

class BinaryLoader : public ResourceLoader {
  public:
    BinaryLoader();
    ~BinaryLoader();

    Resource* load(const String name);
    void      unload(Resource* resource);

  private:
};