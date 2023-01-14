#pragma once

#include <set>

#include "systems/memory/memory_system.hpp"

/**
 *  @brief A standard container made up of unique keys, which can be
 *  retrieved in logarithmic time.
 *
 *  @ingroup associative_containers
 *
 *  @tparam _Key  Type of key objects.
 *  @tparam _Compare  Comparison function object type, defaults to less<_Key>.
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, a
 *  <a href="tables.html#66">reversible container</a>, and an
 *  <a href="tables.html#69">associative container</a> (using unique keys).
 *
 *  Sets support bidirectional iterators.
 *
 *  The private tree data is declared exactly the same way for Set and
 *  Multiset; the distinction is made entirely in how the tree functions are
 *  called (*_unique versus *_equal, same as the standard).
 */
template<typename _Key, typename _Compare = std::less<_Key>>
class Set : public std::set<_Key, _Compare, TAllocator<_Key>> {
  private:
    typedef std::set<_Key, _Compare, TAllocator<_Key>> _base_class;
    typedef std::set<_Key, _Compare>                   default_version;
    typedef TAllocator<_Key>                           allocator_type;
    typedef _Key                                       value_type;

  public:
    using std::set<_Key, _Compare, TAllocator<_Key>>::set;
    using std::set<_Key, _Compare, TAllocator<_Key>>::insert;

    // allocation/deallocation
    /**
     *  @brief  Default constructor creates no elements.
     */
    Set() : _base_class(allocator_type(MemoryTag::Set)) {}

    /**
     *  @brief  Creates a %Set with no elements.
     *  @param  __comp  Comparator to use.
     *  @param  __a  An allocator object.
     */
    explicit Set(
        const _Compare&       __comp,
        const allocator_type& __a = allocator_type(MemoryTag::Set)
    )
        : _base_class(__comp, __a) {}

    /**
     *  @brief  Builds a %Set from a range.
     *  @param  __first  An input iterator.
     *  @param  __last  An input iterator.
     *
     *  Create a %Set consisting of copies of the elements from
     *  [__first,__last).  This is linear in N if the range is
     *  already sorted, and NlogN otherwise (where N is
     *  distance(__first,__last)).
     */
    template<typename _InputIterator>
    Set(_InputIterator __first, _InputIterator __last)
        : _base_class(allocator_type(MemoryTag::Set)) {
        insert(__first, __last);
    }

    /**
     *  @brief  Builds a %Set from a range.
     *  @param  __first  An input iterator.
     *  @param  __last  An input iterator.
     *  @param  __comp  A comparison functor.
     *  @param  __a  An allocator object.
     *
     *  Create a %Set consisting of copies of the elements from
     *  [__first,__last).  This is linear in N if the range is
     *  already sorted, and NlogN otherwise (where N is
     *  distance(__first,__last)).
     */
    template<typename _InputIterator>
    Set(_InputIterator        __first,
        _InputIterator        __last,
        const _Compare&       __comp,
        const allocator_type& __a = allocator_type(MemoryTag::Set))
        : _base_class(__first, __last, __comp, __a) {}

    /**
     *  @brief  %Set copy constructor.
     *
     *  Whether the allocator is copied depends on the allocator traits.
     */
    Set(const default_version& __x)
        : _base_class(allocator_type(MemoryTag::Set)) {
        for (auto var : __x)
            insert(var);
    }

    /**
     *  @brief %Set move constructor
     *
     *  The newly-created %Set contains the exact contents of the moved
     *  instance. The moved instance is a valid, but unspecified, %Set.
     */
    Set(default_version&& __x) : _base_class(MemoryTag::Set) {
        for (auto var : __x)
            insert(var);
    }

    /**
     *  @brief  Builds a %Set from an initializer_list.
     *  @param  __l  An initializer_list.
     *  @param  __comp  A comparison functor.
     *  @param  __a  An allocator object.
     *
     *  Create a %Set consisting of copies of the elements in the list.
     *  This is linear in N if the list is already sorted, and NlogN
     *  otherwise (where N is @a __l.size()).
     */
    Set(std::initializer_list<value_type> __l,
        const _Compare&                   __comp = _Compare(),
        const allocator_type&             __a = allocator_type(MemoryTag::Set))
        : _base_class(__l, __comp, __a) {}
};
