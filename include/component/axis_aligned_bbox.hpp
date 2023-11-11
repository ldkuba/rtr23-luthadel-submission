#pragma once

#include "bounding_box.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Generic representation for n-dimensional axis aligned bounding box.
 * Implements generic BoundingBox
 * @tparam Dim Dimension count
 */
template<uint8 Dim = 2>
class AxisAlignedBBox : public BoundingBox<AxisAlignedBBox<Dim>, Dim> {
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
};

} // namespace ENGINE_NAMESPACE