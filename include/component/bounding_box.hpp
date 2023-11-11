#pragma once

#include "ray.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Generic abstract n-dimensional bounding box class
 * @tparam BBox Type of derived bbox
 * @tparam Dim dimension count
 */
template<typename BBox, uint8 Dim = 2>
class BoundingBox {
  public:
    /// @brief Point type used by this class (Ex. for @a Dim == 3 this will be
    /// @p vec3)
    typedef glm::vec<Dim, float32> Vector;

    BoundingBox() {}
    ~BoundingBox() {}

    // Operators
    virtual bool operator==(const BBox& other) const = 0;

    // Gets
    /// @brief Calculate n-dimensional volume
    virtual float32 get_volume() const       = 0;
    /// @brief Calculate surface area of the [n-1]-dimensional bbox boundary
    virtual float32 get_surface_area() const = 0;
    /// @brief Get center point
    virtual Vector  get_center() const       = 0;
    /// @brief Get index for axis-aligned dimension with the largest associated
    /// length.
    virtual uint32  get_major_axis() const   = 0;
    /// @brief Get index for axis-aligned dimension with the smallest associated
    /// length.
    virtual uint32  get_minor_axis() const   = 0;
    /// @brief Calculate the BBox extents
    virtual Vector  get_extent() const       = 0;

    // Distance
    /**
     * @brief Calculate closest squared distance between this bbox and given
     * point @c p.
     * @param p Point for which distance is computed
     * @return float32 Requested distance
     */
    virtual float32 sq_distance_to(const Vector& p) const  = 0;
    /**
     * @brief Calculate closest squared distance between this bbox and given
     * bbox @c bbox.
     * @param bbox Bounding box for which distance is computed
     * @return float32 Requested distance
     */
    virtual float32 sq_distance_to(const BBox& bbox) const = 0;
    /**
     * @brief Calculate closest distance between this bbox and given point @c p.
     * @param p Point for which distance is computed
     * @return float32 Requested distance
     */
    virtual float32 distance_to(const Vector& p) const     = 0;
    /**
     * @brief Calculate closest distance between this bbox and given @c bbox.
     * @param bbox Bounding box for which distance is computed
     * @return float32 Requested distance
     */
    virtual float32 distance_to(const BBox& bbox) const    = 0;

    // Intersections
    /**
     * @brief Check whether ray intersects this bbox
     * @param ray Ray we are trying to intersect
     * @return true If intersection exists
     * @return false Otherwise
     */
    virtual bool intersect_ray(const Ray<Dim>& ray) const = 0;
    /**
     * @brief Check whether ray intersects this bbox and if it does return
     * intersection distances
     * @param ray Ray we are trying to intersect
     * @param near_t out Distance to the closer intersection
     * @param far_f out Distance to the further intersection
     * @return true If intersection exists
     * @return false Otherwise
     */
    virtual bool intersect_ray(
        const Ray<Dim>& ray, float32& near_t, float32& far_f
    ) const = 0;

    // Checks
    /**
     * @brief Check whether a given point is contain within this bbox. This
     * operation includes bbox boundary if @p strict is set to false.
     * @param p Point on which the check will be preformed
     * @param strict If true includes bbox boundary
     * @return true If point @p p is included
     * @return false Otherwise
     */
    virtual bool contains(const Vector& p, const bool strict = false) const = 0;
    /**
     * @brief Check whether a given bounding box is contain within this bbox.
     * This operation includes bbox boundary if @p strict is set to false.
     * @param bbox Bounding box on which the check will be preformed
     * @param strict If true includes bbox boundary
     * @return true If input @p bbox is included
     * @return false Otherwise
     */
    virtual bool contains(const BBox& bbox, const bool strict = false)
        const = 0;
    /**
     * @brief Check whether a given bounding box overlaps this bbox. This check
     * will include bbox boundary if @p strict is set to false.
     * @param bbox Bounding box on which the check will be preformed
     * @param strict If true includes bbox boundary
     * @return true If input @p bbox overlaps
     * @return false Otherwise
     */
    virtual bool overlaps(const BBox& bbox, const bool strict = false)
        const = 0;

    /// @brief Checks whether this is a valid bounding box
    virtual bool is_valid() const = 0;
    /// @brief Checks whether this bbox is collapsed to a point
    virtual bool is_point() const = 0;

    // Changes
    /**
     * @brief Marks this bounding box as invalid.
     */
    virtual void reset()                     = 0;
    /**
     * @brief Clip this bounding box to another. Basically computes
     * intersection of two bounding boxes.
     * @param bbox Bounding box to clip to
     */
    virtual void clip(const BBox& bbox)      = 0;
    /**
     * @brief Extend this bounding box until it contains point @p p
     * @param p Point which must be contained by this bbox
     */
    virtual void expand_by(const Vector& p)  = 0;
    /**
     * @brief Expand this bounding box until it contains bounding box @p bbox
     * @param bbox Bounding box which must be contained by this bbox
     */
    virtual void expand_by(const BBox& bbox) = 0;
};

} // namespace ENGINE_NAMESPACE