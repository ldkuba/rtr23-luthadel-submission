#pragma once

#include "systems/memory/memory_system.hpp"

#include <list>

namespace ENGINE_NAMESPACE {

/**
 *  @brief A standard container with linear time access to elements,
 *  and fixed time insertion/deletion at any point in the sequence.
 *
 *  @ingroup sequences
 *
 *  @tparam _Tp  Type of element.
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, a
 *  <a href="tables.html#66">reversible container</a>, and a
 *  <a href="tables.html#67">sequence</a>, including the
 *  <a href="tables.html#68">optional sequence requirements</a> with the
 *  %exception of @c at and @c operator[].
 *
 *  This is a @e doubly @e linked %List.  Traversal up and down the
 *  %List requires linear time, but adding and removing elements (or
 *  @e nodes) is done in constant time, regardless of where the
 *  change takes place.  Unlike std::vector and std::deque,
 *  random-access iterators are not provided, so subscripting ( @c
 *  [] ) access is not allowed.  For algorithms which only need
 *  sequential access, this lack makes no difference.
 *
 *  Also unlike the other standard containers, std::list provides
 *  specialized algorithms %unique to linked lists, such as
 *  splicing, sorting, and in-place reversal.
 *
 *  A couple points on memory allocation for list<Tp>:
 *
 *  First, we never actually allocate a Tp, we allocate
 *  List_node<Tp>'s and trust [20.1.5]/4 to DTRT.  This is to ensure
 *  that after elements from %List<X,Alloc1> are spliced into
 *  %List<X,Alloc2>, destroying the memory of the second %List is a
 *  valid operation, i.e., Alloc1 giveth and Alloc2 taketh away.
 *
 *  Second, a %List conceptually represented as
 *  @code
 *    A <---> B <---> C <---> D
 *  @endcode
 *  is actually circular; a link exists between A and D.  The %List
 *  class holds (as its only data member) a private list::iterator
 *  pointing to @e D, not to @e A!  To get to the head of the %List,
 *  we start at the tail and move forward by one.  When this member
 *  iterator's next/previous pointers refer to itself, the %List is
 *  %empty.
 */
template<typename _Tp>
class List : public std::list<_Tp, TAllocator<_Tp>> {
  private:
    typedef std::list<_Tp, TAllocator<_Tp>> _base_class;
    typedef std::list<_Tp>                  default_version;
    typedef TAllocator<_Tp>                 allocator_t;

  public:
    using std::list<_Tp, TAllocator<_Tp>>::list;
    using std::list<_Tp, TAllocator<_Tp>>::push_back;

    /**
     *  @brief  Creates a %List with no elements.
     */
    List() : _base_class(allocator_t(MemoryTag::List)) {}

    /**
     *  @brief  Creates a %List with default constructed elements.
     *  @param  __n  The number of elements to initially create.
     *  @param  __a  An allocator object.
     *
     *  This constructor fills the %List with @a __n default
     *  constructed elements.
     */
    explicit List(
        uint64 __n, const allocator_t& __a = allocator_t(MemoryTag::List)
    )
        : _base_class(__n, __a) {}

    /**
     *  @brief  Creates a %List with copies of an exemplar element.
     *  @param  __n  The number of elements to initially create.
     *  @param  __value  An element to copy.
     *  @param  __a  An allocator object.
     *
     *  This constructor fills the %List with @a __n copies of @a
     __value.
     */
    List(
        uint64             __n,
        const _Tp&         __value,
        const allocator_t& __a = allocator_t(MemoryTag::List)
    )
        : _base_class(__n, __value, __a) {}

    /**
     *  @brief  %List copy constructor.
     *  @param  __x  A %List of identical element and allocator types.
     *
     *  The newly-created %List uses a copy of the allocation object
     used
     *  by @a __x (unless the allocator traits dictate a different
     object).
     */
    List(const default_version& __x)
        : _base_class(allocator_t(MemoryTag::List)) {
        for (auto var : __x)
            push_back(var);
    }

    /**
     *  @brief  %List move constructor.
     *
     *  The newly-created %List contains the exact contents of the
     moved
     *  instance. The contents of the moved instance are a valid, but
     *  unspecified %List.
     */
    List(default_version&& __x) : _base_class(allocator_t(MemoryTag::List)) {
        for (auto var : __x)
            push_back(var);
    }

    /**
     *  @brief  Builds a %List from an initializer_list
     *  @param  __l  An initializer_list of value_type.
     *  @param  __a  An allocator object.
     *
     *  Create a %List consisting of copies of the elements in the
     *  initializer_list @a __l.  This is linear in __l.size().
     */
    List(
        std::initializer_list<_Tp> __l,
        const allocator_t&         __a = allocator_t(MemoryTag::List)
    )
        : _base_class(__l, __a) {}

    /**
     *  @brief  Builds a %List from a range.
     *  @param  __first  An input iterator.
     *  @param  __last  An input iterator.
     *  @param  __a  An allocator object.
     *
     *  Create a %List consisting of copies of the elements from
     *  [@a __first,@a __last).  This is linear in N (where N is
     *  distance(@a __first,@a __last)).
     */
    template<
        typename _InputIterator,
        typename = std::_RequireInputIter<_InputIterator>>
    List(
        _InputIterator     __first,
        _InputIterator     __last,
        const allocator_t& __a = allocator_t(MemoryTag::List)
    )
        : _base_class(__first, __last, __a) {}
};

} // namespace ENGINE_NAMESPACE