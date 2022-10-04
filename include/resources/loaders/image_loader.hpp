#ifndef __IMAGE_LOADER_H__
#define __IMAGE_LOADER_H__

#pragma once

#include "resource_loader.hpp"

class ImageLoader : public ResourceLoader {
public:
    ImageLoader();
    ~ImageLoader();

    Resource* load(const String name);
    void unload(Resource* resource);

private:

};
#endif // __IMAGE_LOADER_H__