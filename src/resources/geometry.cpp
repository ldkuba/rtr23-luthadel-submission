#include "resources/geometry.hpp"

namespace ENGINE_NAMESPACE {

// Statics values
const uint32 Geometry::max_name_length;

Geometry::Geometry(String name) : Resource(name) {}
Geometry::~Geometry() {}

} // namespace ENGINE_NAMESPACE