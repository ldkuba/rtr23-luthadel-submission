#pragma once

#include <vector>

#include "memory_system.hpp"

template<typename Tp>
class Vector : public std::vector<Tp, TAllocator<Tp>> {
  private:
    typedef std::vector<Tp, TAllocator<Tp>>             _base_class;
    typedef TAllocator<Tp>                              t_allocator_type;
    typedef std::_Vector_base<Tp, t_allocator_type>     _super_base;
    typedef typename _super_base::_Tp_alloc_type        _Tp_t_alloc_type;
    typedef __gnu_cxx::__alloc_traits<_Tp_t_alloc_type> _TAlloc_traits;

  public:
    using std::vector<Tp, TAllocator<Tp>>::vector;
    using std::vector<Tp, TAllocator<Tp>>::reserve;

    /**
     *  @brief  Creates a %vector with no elements.
     */
    Vector() : _base_class(t_allocator_type(MemoryTag::Array)) {
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

    // Copy constructors
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
    Vector(const std::vector<Tp, t_allocator_type>& __x) : _base_class(__x) {}

    Vector<Tp>& operator=(const std::vector<Tp>& __x) {
        reserve(__x.size());
        for (auto var : __x)
            this->emplace_back(var);
        return *this;
    }

    Vector(const std::vector<Tp>& __x) : Vector() {
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
    // Vector(std::vector<Tp, t_allocator_type>&&) noexcept = default;

    /// Copy constructor with alternative allocator
    Vector(
        const std::vector<Tp, t_allocator_type>& __x,
        const t_allocator_type&                  __a
    )
        : _base_class(__x, __a) {}

    // /// Move constructor with alternative allocator
    // Vector(std::vector<Tp>&& __rv, const t_allocator_type& __m) noexcept(
    //     noexcept(vector(
    //         std::declval<std::vector<Tp>&&>(),
    //         std::declval<const t_allocator_type&>(),
    //         std::declval<typename _TAlloc_traits::is_always_equal>()
    //     ))
    // )
    //     : _base_class(__rv, __m) {}
};
