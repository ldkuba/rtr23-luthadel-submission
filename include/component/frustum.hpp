#pragma once

#include "axis_aligned_bbox.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief View frustum. Defines extents of a given view. Used for view frustum
 * based culling.
 */
class Frustum {
  private:
    struct Plane {
        glm::vec4 equation;

        Plane();
        Plane(const glm::vec3 position, const glm::vec3 normal);

        float32 signed_distance(const glm::vec3 position) const;

        template<uint8 D>
        bool contains(const AxisAlignedBBox<D>& aabb) const {
            Logger::fatal("Unimplemented.");
            return false;
        }
    };

  public:
    /**
     * @brief Construct a new Frustum object
     * @param position Perspective frustum origin
     * @param forward Forward direction vector
     * @param right Right direction vector
     * @param up Up direction vector
     * @param aspect_ratio View aspect ratio
     * @param field_of_view View field of view
     * @param near Near plane distance
     * @param far Far plane distance
     */
    Frustum(
        const glm::vec3 position,
        const glm::vec3 forward,
        const glm::vec3 right,
        const glm::vec3 up,
        const float32   aspect_ratio,
        const float32   field_of_view,
        const float32   near,
        const float32   far
    );
    ~Frustum();

    /**
     * @brief Check whether this frustum contains or intersects a given
     * axis-aligned bounding box.
     */
    template<uint8 D>
    bool contains(const AxisAlignedBBox<D>& aabb) const {
        for (const auto& side : _sides)
            if (!side.contains(aabb)) return false;
        return true;
    }

  private:
    std::array<Plane, 6> _sides {};
};

} // namespace ENGINE_NAMESPACE