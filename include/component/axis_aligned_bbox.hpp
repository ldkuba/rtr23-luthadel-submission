#pragma once

#include "bounding_box.hpp"
#include "serialization/serializer.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Generic representation for n-dimensional axis aligned bounding box.
 * Implements generic BoundingBox
 * @tparam Dim Dimension count
 */
template<uint8 Dim = 2>
class AxisAlignedBBox : public BoundingBox<AxisAlignedBBox<Dim>, Dim>,
                        public Serializable {
  public:
    typedef glm::vec<Dim, float32> Vector;
    typedef AxisAlignedBBox<Dim>   BBox;

    // Min & Max
    Vector min;
    Vector max;

    /**
     * @brief Construct a new invalid Axis Aligned Bounding Box object. Invalid
     * box has no width or height, and wont contain, intersect or interact with
     * anything.
     */
    AxisAlignedBBox();
    /**
     * @brief Construct new Axis Aligned Bounding Box object. Object created
     * with this method will be collapsed into a single point, with width and
     * height of 0.
     * @param p
     */
    AxisAlignedBBox(const Vector& p);
    /**
     * @brief Construct a new Axis Aligned Bounding Box object from two
     * positions.
     * @param min BBox min. Only valid if all components < @p max
     * @param max BBox max. Only valid if all components > @p min
     */
    AxisAlignedBBox(const Vector& min, const Vector& max);
    ~AxisAlignedBBox();

    // Operators
    virtual bool operator==(const BBox& other) const override {
        return min == other.min && max == other.max;
    }

    // Gets
    virtual float32 get_volume() const override;
    virtual float32 get_surface_area() const override;
    virtual Vector  get_center() const override;
    virtual uint32  get_major_axis() const override;
    virtual uint32  get_minor_axis() const override;
    virtual Vector  get_extent() const override;

    // Distance
    virtual float32 sq_distance_to(const Vector& p) const override;
    virtual float32 sq_distance_to(const BBox& bbox) const override;
    virtual float32 distance_to(const Vector& p) const override;
    virtual float32 distance_to(const BBox& bbox) const override;

    // Intersections
    virtual bool intersect_ray(const Ray<Dim>& ray) const override;
    virtual bool intersect_ray(
        const Ray<Dim>& ray, float32& near_t, float32& far_f
    ) const override;

    // Checks
    virtual bool contains(const Vector& p, const bool strict = false)
        const override;
    virtual bool contains(const BBox& bbox, const bool strict = false)
        const override;
    virtual bool overlaps(const BBox& bbox, const bool strict = false)
        const override;

    virtual bool is_valid() const override;
    virtual bool is_point() const override;

    // Changes
    virtual void reset() override;
    virtual void clip(const BBox& bbox) override;
    virtual void expand_by(const Vector& p) override;
    virtual void expand_by(const BBox& bbox) override;

    serializable_attributes(min, max);
};

template<uint8 Dim>
AxisAlignedBBox<Dim>::AxisAlignedBBox() {
    reset();
}
template<uint8 Dim>
AxisAlignedBBox<Dim>::AxisAlignedBBox(const Vector& p) : min(p), max(p) {}
template<uint8 Dim>
AxisAlignedBBox<Dim>::AxisAlignedBBox(const Vector& min, const Vector& max)
    : min(min), max(max) {}

template<uint8 Dim>
AxisAlignedBBox<Dim>::~AxisAlignedBBox() {}

// //////////////////////////////// //
// AXIS ALIGNED BBOX PUBLIC METHODS //
// //////////////////////////////// //

// Getters
template<uint8 Dim>
float32 AxisAlignedBBox<Dim>::get_volume() const {
    const auto dif    = max - min;
    float32    volume = 1.f;
    for (uint8 i = 0; i < Dim; i++)
        volume *= dif[i];
    return volume;
}
template<uint8 Dim>
float32 AxisAlignedBBox<Dim>::get_surface_area() const {
    const auto dif = max - min;

    float32 area = 0.0f;
    for (uint8 i = 0; i < Dim; ++i) {
        float32 term = 1.0f;
        for (int8 j = 0; j < Dim; ++j) {
            if (i == j) continue;
            term *= dif[j];
        }
        area += term;
    }
    area *= 2;

    return area;
}
template<uint8 Dim>
glm::vec<Dim, float32> AxisAlignedBBox<Dim>::get_center() const {
    return (max + min) * 0.5f;
}
template<uint8 Dim>
uint32 AxisAlignedBBox<Dim>::get_major_axis() const {
    Vector dis = max - min;

    int8 largest = 0;
    for (int8 i = 1; i < Dim; ++i)
        if (dis[i] > dis[largest]) largest = i;

    return largest;
}
template<uint8 Dim>
uint32 AxisAlignedBBox<Dim>::get_minor_axis() const {
    Vector dis = max - min;

    int8 shortest = 0;
    for (int8 i = 1; i < Dim; ++i)
        if (dis[i] < dis[shortest]) shortest = i;

    return shortest;
}
template<uint8 Dim>
glm::vec<Dim, float32> AxisAlignedBBox<Dim>::get_extent() const {
    return max - min;
}

// Distance
template<uint8 Dim>
float32 AxisAlignedBBox<Dim>::sq_distance_to(const Vector& p) const {
    float32 sq_dist = 0;

    for (int8 i = 0; i < Dim; ++i) {
        const auto value = (p[i] < min[i])   ? min[i] - p[i]
                           : (p[i] > max[i]) ? p[i] - max[i]
                                             : 0.f;
        sq_dist += value * value;
    }

    return sq_dist;
}
template<uint8 Dim>
float32 AxisAlignedBBox<Dim>::sq_distance_to(const BBox& bbox) const {
    float32 sq_dist = 0;

    for (int8 i = 0; i < Dim; ++i) {
        float32 value = (bbox.max[i] < min[i])   ? min[i] - bbox.max[i]
                        : (bbox.min[i] > max[i]) ? bbox.min[i] - max[i]
                                                 : 0.f;
        sq_dist += value * value;
    }

    return sq_dist;
}
template<uint8 Dim>
float32 AxisAlignedBBox<Dim>::distance_to(const Vector& p) const {
    return std::sqrt(sq_distance_to(p));
}
template<uint8 Dim>
float32 AxisAlignedBBox<Dim>::distance_to(const BBox& bbox) const {
    return std::sqrt(sq_distance_to(bbox));
}

// Intersection
template<uint8 Dim>
bool AxisAlignedBBox<Dim>::intersect_ray(const Ray<Dim>& ray) const {
    float32 n, t;
    return intersect_ray(ray, n, t);
}
template<uint8 Dim>
bool AxisAlignedBBox<Dim>::intersect_ray(
    const Ray<Dim>& ray, float32& near_t, float32& far_t
) const {
    near_t = -Infinity32;
    far_t  = Infinity32;

    for (int8 i = 0; i < 3; i++) {
        const auto origin  = ray.origin[i];
        const auto min_val = min[i];
        const auto max_val = max[i];

        if (ray.direction[i] == 0) {
            if (origin < min_val || origin > max_val) return false;
        } else {
            auto t1 = (min_val - origin) / ray.direction[i];
            auto t2 = (max_val - origin) / ray.direction[i];

            if (t1 > t2) std::swap(t1, t2);

            near_t = std::max(t1, near_t);
            far_t  = std::min(t2, far_t);

            if (!(near_t <= far_t)) return false;
        }
    }

    return ray.min_t <= far_t && near_t <= ray.max_t;
}

// Checks
template<uint8 Dim>
bool AxisAlignedBBox<Dim>::contains(const Vector& p, const bool strict) const {
    if (strict) {
        return glm::all(glm::greaterThan(p, min)) &&
               glm::all(glm::lessThan(p, max));
    }
    return glm::all(glm::greaterThanEqual(p, min)) &&
           glm::all(glm::lessThanEqual(p, max));
}
template<uint8 Dim>
bool AxisAlignedBBox<Dim>::contains(const BBox& bbox, const bool strict) const {
    if (strict) {
        return glm::all(glm::greaterThan(bbox.min, min)) &&
               glm::all(glm::lessThan(bbox.max, max));
    }
    return glm::all(glm::greaterThanEqual(bbox.min, min)) &&
           glm::all(glm::lessThanEqual(bbox.max, max));
}
template<uint8 Dim>
bool AxisAlignedBBox<Dim>::overlaps(const BBox& bbox, const bool strict) const {
    if (strict) {
        return glm::all(glm::lessThan(bbox.min, max)) &&
               glm::all(glm::greaterThan(bbox.max, min));
    }
    return glm::all(glm::lessThanEqual(bbox.min, max)) &&
           glm::all(glm::greaterThanEqual(bbox.max, min));
}

template<uint8 Dim>
bool AxisAlignedBBox<Dim>::is_valid() const {
    return glm::all(glm::greaterThanEqual(max, min));
}
template<uint8 Dim>
bool AxisAlignedBBox<Dim>::is_point() const {
    return glm::all(glm::epsilonEqual(max, min, Epsilon32));
}

// Changes
template<uint8 Dim>
void AxisAlignedBBox<Dim>::reset() {
    min = Vector(Infinity32);
    max = Vector(-Infinity32);
}
template<uint8 Dim>
void AxisAlignedBBox<Dim>::clip(const BBox& bbox) {
    min = glm::max(min, bbox.min);
    max = glm::min(max, bbox.max);
}
template<uint8 Dim>
void AxisAlignedBBox<Dim>::expand_by(const Vector& p) {
    min = glm::min(min, p);
    max = glm::max(max, p);
}
template<uint8 Dim>
void AxisAlignedBBox<Dim>::expand_by(const BBox& bbox) {
    min = glm::min(min, bbox.min);
    max = glm::max(max, bbox.max);
}

} // namespace ENGINE_NAMESPACE