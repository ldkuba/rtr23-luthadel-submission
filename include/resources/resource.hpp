#ifndef __RESOURCE_H__
#define __RESOURCE_H__

#pragma once

#include "string.hpp"
#include "property.hpp"
#include "logger.hpp"

#include <optional>

class Resource {
public:
    /// @brief Unique texture identifier
    std::optional<uint64> id;
    /// @brief ...
    Property<String> name{ Get { return _name; } };
    /// @brief ...
    Property<String> full_path{
        Get { return _full_path; } ,
        Set { _full_path = value; }
    };
    /// @brief ...
    Property<String> loader_type{
        Get { return _loader_type; },
        Set {
            if (_loader_type == "")
                _loader_type = value;
            else
                Logger::error("Resource :: Loader type cannot be set after initialization.");
        }
    };

    Resource(String name);
    ~Resource();


private:
    String _name = "";
    String _full_path = "";
    String _loader_type = "";
};

#endif // __RESOURCE_H__