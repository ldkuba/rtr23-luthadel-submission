#pragma once

#include "defines.hpp"
#include "math_libs.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Generic representation of a ray in @a Dim dimensions.
 * @tparam Dim Dimension count
 */
template<uint8 Dim = 2>
struct Ray {
  public:
    /// @brief Point type used by this class (Ex. for @a Dim == 3 this will be
    /// @p vec3 )
    typedef glm::vec<Dim, float32> Vector;

    /// @brief Origin location from which ray is shot
    Vector origin;
    /// @brief Direction in which this ray is pointing
    Vector direction;

    /// @brief Minimal param position on this ray
    float32 min_t;
    /// @brief Maximal param position on this ray
    float32 max_t;

    Ray();
    ~Ray();

    Vector operator()(const float32& t) { return origin + t * direction; }
    Vector operator()(float32&& t) { return origin + t * direction; }

  private:
};

} // namespace ENGINE_NAMESPACE