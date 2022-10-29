#pragma once

#include "memory_system.hpp"

#include <forward_list>

/**
 *  @brief A standard container with linear time access to elements,
 *  and fixed time insertion/deletion at any point in the sequence.
 *
 *  @ingroup sequences
 *
 *  @tparam _Tp  Type of element.
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, a
 *  <a href="tables.html#67">sequence</a>, including the
 *  <a href="tables.html#68">optional sequence requirements</a> with the
 *  %exception of @c at and @c operator[].
 *
 *  This is a @e singly @e linked %list.  Traversal up the
 *  %list requires linear time, but adding and removing elements (or
 *  @e nodes) is done in constant time, regardless of where the
 *  change takes place.  Unlike std::vector and std::deque,
 *  random-access iterators are not provided, so subscripting ( @c
 *  [] ) access is not allowed.  For algorithms which only need
 *  sequential access, this lack makes no difference.
 *
 *  Also unlike the other standard containers, std::ForwardList provides
 *  specialized algorithms %unique to linked lists, such as
 *  splicing, sorting, and in-place reversal.
 */
template<typename _Tp>
class ForwardList : public std::forward_list<_Tp, TAllocator<_Tp>> {
  private:
    typedef std::forward_list<_Tp, TAllocator<_Tp>> _base_class;
    typedef std::forward_list<_Tp>                  default_version;
    typedef TAllocator<_Tp>                         allocator_t;

  public:
    using std::forward_list<_Tp, TAllocator<_Tp>>::forward_list;
    using std::forward_list<_Tp, TAllocator<_Tp>>::push_front;
    using std::forward_list<_Tp, TAllocator<_Tp>>::reverse;

    /**
     *  @brief  Creates a %ForwardList with no elements.
     */
    ForwardList() : _base_class(allocator_t(MemoryTag::List)) {}

    /**
     *  @brief  Creates a %ForwardList with default constructed elements.
     *  @param  __n   The number of elements to initially create.
     *  @param  __al  An allocator object.
     *
     *  This constructor creates the %ForwardList with @a __n default
     *  constructed elements.
     */
    explicit ForwardList(
        uint64 __n, const allocator_t& __al = allocator_t(MemoryTag::List)
    )
        : _base_class(__n, __al) {}

    /**
     *  @brief  Creates a %ForwardList with copies of an exemplar element.
     *  @param  __n      The number of elements to initially create.
     *  @param  __value  An element to copy.
     *  @param  __al     An allocator object.
     *
     *  This constructor fills the %ForwardList with @a __n copies of
     *  @a __value.
     */
    ForwardList(
        uint64             __n,
        const _Tp&         __value,
        const allocator_t& __al = allocator_t(MemoryTag::List)
    )
        : _base_class(__n, __value, __al) {}

    /**
     *  @brief  Builds a %ForwardList from a range.
     *  @param  __first  An input iterator.
     *  @param  __last   An input iterator.
     *  @param  __al     An allocator object.
     *
     *  Create a %ForwardList consisting of copies of the elements from
     *  [@a __first,@a __last).  This is linear in N (where N is
     *  distance(@a __first,@a __last)).
     */
    template<
        typename _InputIterator,
        typename = std::_RequireInputIter<_InputIterator>>
    ForwardList(
        _InputIterator     __first,
        _InputIterator     __last,
        const allocator_t& __al = allocator_t(MemoryTag::List)
    )
        : _base_class(__first, __last, __al) {}

    /**
     *  @brief  The %ForwardList copy constructor.
     *  @param  __list  A %ForwardList of identical element and allocator
     *                  types.
     */
    ForwardList(const default_version& __list)
        : _base_class(allocator_t(MemoryTag::List)) {
        for (auto var : __list)
            push_front(var);
        reverse();
    }

    /**
     *  @brief  The %ForwardList move constructor.
     *  @param  __list  A %ForwardList of identical element and allocator
     *                  types.
     *
     *  The newly-created %ForwardList contains the exact contents of the
     *  moved instance. The contents of the moved instance are a valid, but
     *  unspecified %ForwardList.
     */
    ForwardList(default_version&& __list)
        : _base_class(allocator_t(MemoryTag::List)) {
        for (auto var : __list)
            push_front(var);
        reverse();
    }

    /**
     *  @brief  Builds a %ForwardList from an initializer_list
     *  @param  __il  An initializer_list of value_type.
     *  @param  __al  An allocator object.
     *
     *  Create a %ForwardList consisting of copies of the elements
     *  in the initializer_list @a __il.  This is linear in __il.size().
     */
    ForwardList(
        std::initializer_list<_Tp> __il,
        const allocator_t&         __al = allocator_t(MemoryTag::List)
    )
        : _base_class(__il, __al) {}
};
