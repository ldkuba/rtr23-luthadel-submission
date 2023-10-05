#pragma once

#include "resource_loader.hpp"
#include "resources/geometry.hpp"

namespace ENGINE_NAMESPACE {

// TODO: TEMP
class GeometryConfig;

/**
 * @brief Resource loader that handles mesh data.
 *
 */
class MeshLoader : public ResourceLoader {
  public:
    MeshLoader();
    ~MeshLoader();

    Result<Resource*, RuntimeError> load(const String name) override;
    void                            unload(Resource* resource) override;

  private:
    struct MeshFileType {
        const String extension;
        const bool   binary;
        Result<GeometryConfigArray*, RuntimeError> //
            (*const load)(const String&, const String&);
    };

    static const std::vector<MeshFileType> _supported_mesh_file_types;
};

} // namespace ENGINE_NAMESPACE