#pragma once

#include "resources/resource.hpp"
#include "result.hpp"
#include "error_types.hpp"

#define RESOURCE_LOG "ResourceLoader :: "

/**
 * @brief Predefined resourse types
 */
struct ResourceType {
    STRING_CONST(Text);
    STRING_CONST(Binary);
    STRING_CONST(Image);
    STRING_CONST(Material);
    STRING_CONST(StaticMesh);
    STRING_CONST(Shader);
    static bool is_custom(const String type) {
        return (
            (type.compare_ci(Text) != 0) && (type.compare_ci(Binary) != 0) &&
            (type.compare_ci(Image) != 0) && (type.compare_ci(Material) != 0) &&
            (type.compare_ci(StaticMesh) != 0) && (type.compare_ci(Shader) != 0)
        );
    }
};

/**
 * @brief Generic resource loader interface. Used for loading/unloading of a
 * specific asset type form the assets folder.
 */
class ResourceLoader {
  public:
    /// @brief Resource type loaded by this resource / Resource Loader type name
    Property<String> type {
        GET { return _type; }
    };

    /**
     * @brief Construct a new Resource Loader object
     */
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