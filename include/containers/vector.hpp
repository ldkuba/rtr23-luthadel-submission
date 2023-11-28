#pragma once

#include <vector>

#include "systems/memory/memory_system.hpp"

namespace ENGINE_NAMESPACE {

/**
 *  @brief A standard container which offers fixed time access to
 *  individual elements in any order.
 *
 *  @ingroup sequences
 *
 *  @tparam _Tp  Type of element.
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, a
 *  <a href="tables.html#66">reversible container</a>, and a
 *  <a href="tables.html#67">sequence</a>, including the
 *  <a href="tables.html#68">optional sequence requirements</a> with the
 *  %exception of @c push_front and @c pop_front.
 *
 *  In some terminology a %vector can be described as a dynamic
 *  C-style array, it offers fast and efficient access to individual
 *  elements in any order and saves the user from worrying about
 *  memory and size allocation.  Subscripting ( @c [] ) access is
 *  also provided as with C-style arrays.
 */
template<typename Tp>
class Vector : public std::vector<Tp, TAllocator<Tp>> {
  private:
    typedef std::vector<Tp, TAllocator<Tp>>                    _base_class;
    typedef std::vector<Tp>                                    default_version;
    typedef TAllocator<Tp>                                     t_allocator_type;
    typedef std::_Vector_base<Tp, t_allocator_type> _super_base;
    typedef typename _super_base::_Tp_alloc_type               _Tp_t_alloc_type;
    typedef __gnu_cxx::__alloc_traits<_Tp_t_alloc_type>        _TAlloc_traits;

  public:
    using std::vector<Tp, TAllocator<Tp>>::vector;
    using std::vector<Tp, TAllocator<Tp>>::reserve;

    /**
     *  @brief  Creates a %vector with no elements.
     *  @param  __a  An allocator.
     */
    Vector(const TAllocator<Tp>& __a = t_allocator_type(MemoryTag::Array))
        : _base_class(__a) {
        // Reserve at least 16 bytes
        reserve(15 / sizeof(Tp) + 1);
    }

    /**
     *  @brief  Creates a %vector with default constructed elements.
     *  @param  __n  The number of elements to initially create.
     *  @param  __a  An allocator.
     *
     *  This constructor fills the %vector with @a __n default
     *  constructed elements.
     */
    explicit Vector(
        uint64                __n,
        const TAllocator<Tp>& __a = t_allocator_type(MemoryTag::Array)
    )
        : _base_class(__n, __a) {}

    /**
     *  @brief  Creates a %vector with copies of an exemplar element.
     *  @param  __n  The number of elements to initially create.
     *  @param  __value  An element to copy.
     *  @param  __a  An allocator.
     *
     *  This constructor fills the %vector with @a __n copies of @a __value.
     */
    Vector(
        uint64                  __n,
        const Tp&               __value,
        const t_allocator_type& __a = t_allocator_type(MemoryTag::Array)
    )
        : _base_class(__n, __value, __a) {}

    /**
     *  @brief  %Vector copy constructor.
     *  @param  __x  A %vector of identical element and allocator types.
     *
     *  All the elements of @a __x are copied, but any unused capacity in
     *  @a __x  will not be copied
     *  (i.e. capacity() == size() in the new %vector).
     *
     *  The newly-created %vector uses a copy of the allocator object used
     *  by @a __x (unless the allocator traits dictate a different object).
     */
    Vector(const default_version& __x)
        : _base_class(t_allocator_type(MemoryTag::Array)) {
        reserve(__x.size());
        for (auto var : __x)
            this->emplace_back(var);
    }

    /**
     *  @brief  %Vector move constructor.
     *
     *  The newly-created %vector contains the exact contents of the
     *  moved instance.
     *  The contents of the moved instance are a valid, but unspecified
     *  %vector.
     */
    Vector(default_version&& __x)
        : _base_class(t_allocator_type(MemoryTag::Array)) {
        reserve(__x.size());
        for (auto var : __x)
            this->emplace_back(var);
    }

    /**
     *  @brief  Builds a %vector from an initializer list.
     *  @param  __l  An initializer_list.
     *  @param  __a  An allocator.
     *
     *  Create a %vector consisting of copies of the elements in the
     *  initializer_list @a __l.
     *
     *  This will call the element type's copy constructor N times
     *  (where N is @a __l.size()) and do no memory reallocation.
     */
    Vector(
        std::initializer_list<Tp> __l,
        const t_allocator_type&   __a = t_allocator_type(MemoryTag::Array)
    )
        : _base_class(__l, __a) {}

    /**
     *  @brief  Builds a %vector from a range.
     *  @param  __first  An input iterator.
     *  @param  __last  An input iterator.
     *  @param  __a  An allocator.
     *
     *  Create a %vector consisting of copies of the elements from
     *  [first,last).
     *
     *  If the iterators are forward, bidirectional, or
     *  random-access, then this will call the elements' copy
     *  constructor N times (where N is distance(first,last)) and do
     *  no memory reallocation.  But if only input iterators are
     *  used, then this will do at most 2N calls to the copy
     *  constructor, and logN memory reallocations.
     */
    template<
        typename _InputIterator,
        typename = std::_RequireInputIter<_InputIterator>>
    Vector(
        _InputIterator          __first,
        _InputIterator          __last,
        const t_allocator_type& __a = t_allocator_type(MemoryTag::Array)
    )
        : _base_class(__first, __last, __a) {}
};

} // namespace ENGINE_NAMESPACE