#pragma once

#include "resources/resource.hpp"
#include "result.hpp"
#include "error_types.hpp"

#define RESOURCE_LOG "ResourceLoader :: "

struct ResourceType {
    constexpr static const char* const Text       = "Text";
    constexpr static const char* const Binary     = "Binary";
    constexpr static const char* const Image      = "Image";
    constexpr static const char* const Material   = "Material";
    constexpr static const char* const StaticMesh = "StaticMesh";
    static bool                        is_custom(const String type) {
        return (
            (type.compare_ci(Text) != 0) && (type.compare_ci(Binary) != 0) &&
            (type.compare_ci(Image) != 0) && (type.compare_ci(Material) != 0) &&
            (type.compare_ci(StaticMesh) != 0)
        );
    }
};

class ResourceLoader {
  public:
    /// @brief Resource type loaded by this resource / Resource Loader type name
    Property<String> type {
        Get { return _type; }
    };

    ResourceLoader() {}
    virtual ~ResourceLoader() {}

    /**
     * @brief Loads requested resource from the asset folder
     *
     * @param name Resource name / file name
     * @returns Requested Resource*
     * @throws FileSystemException if load fails for some reason
     */
    virtual Result<Resource*, RuntimeError> load(const String name) {
        return nullptr;
    }
    /**
     * @brief Releases resource data
     *
     * @param resource Resource to be unloaded
     */
    virtual void unload(Resource* resource) {}

  protected:
    String _type_path;
    String _type;
};

// Helper define
#define CAN_UNLOAD(TYPE, RESOURCE)                                             \
    if (!RESOURCE) {                                                           \
        Logger::warning(                                                       \
            RESOURCE_LOG,                                                      \
            "TYPE unload method called with nullptr. ",                        \
            "Nothing was done"                                                 \
        );                                                                     \
        return;                                                                \
    }                                                                          \
                                                                               \
    if (RESOURCE->loader_type().compare_ci(ResourceType::TYPE) != 0) {         \
        Logger::error(                                                         \
            RESOURCE_LOG,                                                      \
            "TYPE loader used for \"",                                         \
            RESOURCE->loader_type(),                                           \
            "\" unloading."                                                    \
        );                                                                     \
        return;                                                                \
    }