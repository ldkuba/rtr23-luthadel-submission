#pragma once

#include "logger.hpp"
#include "property.hpp"
#include "string.hpp"

#include <optional>

/**
 * @brief Abstract resource class. For a resource to be loadable with a resource
 * loader it needs to extend this class.
 */
class Resource {
  public:
    /// @brief Unique resource identifier
    std::optional<uint64> id;
    /// @brief Resource name
    Property<String>      name {
        GET { return _name; }
    };
    /// @brief Full resource file path
    Property<String> full_path {
        GET { return _full_path; }
        SET {
            if (_full_path == "") _full_path = value;
            else
                Logger::error("Resource :: File path cannot be set after "
                              "initialization.");
        }
    };
    /// @brief Loader used for loading this resource
    Property<String> loader_type {
        GET { return _loader_type; }
        SET {
            if (_loader_type == "") _loader_type = value;
            else
                Logger::error("Resource :: Loader type cannot be set after "
                              "initialization.");
        }
    };

    /**
     * @brief Construct a new Resource object
     *
     * @param name Resource name. Most likely a nonempty string.
     */
    Resource(String name);
    virtual ~Resource();

  private:
    String _name        = "";
    String _full_path   = "";
    String _loader_type = "";
};