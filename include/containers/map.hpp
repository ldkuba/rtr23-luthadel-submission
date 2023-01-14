#pragma once

#include <map>

#include "systems/memory/memory_system.hpp"

/**
 *  @brief A standard container made up of (key,value) pairs, which can be
 *  retrieved based on a key, in logarithmic time.
 *
 *  @ingroup associative_containers
 *
 *  @tparam _Key  Type of key objects.
 *  @tparam  _Tp  Type of Mapped objects.
 *  @tparam _Compare  Comparison function object type, defaults to less<_Key>.
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, a
 *  <a href="tables.html#66">reversible container</a>, and an
 *  <a href="tables.html#69">associative container</a> (using unique keys).
 *  For a @c Map<Key,T> the key_type is Key, the Mapped_type is T, and the
 *  value_type is std::pair<const Key,T>.
 *
 *  Maps support bidirectional iterators.
 *
 *  The private tree data is declared exactly the same way for Map and
 *  multiMap; the distinction is made entirely in how the tree functions are
 *  called (*_unique versus *_equal, same as the standard).
 */
template<
    typename _Key,
    typename _Tp,
    typename _Compare = std::less<_Key>,
    typename _Alloc   = std::allocator<std::pair<const _Key, _Tp>>>
class Map : public std::map<_Key, _Tp, _Compare, TAllocator<_Tp>> {
  private:
    typedef TAllocator<_Tp>                                allocator_type;
    typedef std::map<_Key, _Tp, _Compare, TAllocator<_Tp>> _base_class;
    typedef std::map<_Key, _Tp, _Compare>                  default_version;
    typedef std::pair<const _Key, _Tp>                     value_type;

  public:
    using std::map<_Key, _Tp, _Compare, TAllocator<_Tp>>::map;
    using std::map<_Key, _Tp, _Compare, TAllocator<_Tp>>::insert;

    /**
     *  @brief  Default constructor creates no elements.
     */
    Map() : Map(allocator_type(MemoryTag::Map)) {}

    /**
     *  @brief  Creates a %Map with no elements.
     *  @param  __comp  A comparison object.
     *  @param  __a  An allocator object.
     */
    explicit Map(
        const _Compare&       __comp,
        const allocator_type& __a = allocator_type(MemoryTag::Map)
    )
        : _base_class(__comp, __a) {}

    /**
     *  @brief  %Map copy constructor.
     *
     *  Whether the allocator is copied depends on the allocator traits.
     */
    Map(const default_version& __x)
        : _base_class(allocator_type(MemoryTag::Map)) {
        for (auto var : __x)
            insert(var);
    }

    /**
     *  @brief  %Map move constructor.
     *
     *  The newly-created %Map contains the exact contents of the moved
     *  instance. The moved instance is a valid, but unspecified, %Map.
     */
    Map(default_version&& __x) : _base_class(allocator_type(MemoryTag::Map)) {
        for (auto var : __x)
            insert(var);
    };

    /**
     *  @brief  Builds a %Map from an initializer_list.
     *  @param  __l  An initializer_list.
     *  @param  __comp  A comparison object.
     *  @param  __a  An allocator object.
     *
     *  Create a %Map consisting of copies of the elements in the
     *  initializer_list @a __l.
     *  This is linear in N if the range is already sorted, and NlogN
     *  otherwise (where N is @a __l.size()).
     */
    Map(std::initializer_list<value_type> __l,
        const _Compare&                   __comp = _Compare(),
        const allocator_type&             __a = allocator_type(MemoryTag::Map))
        : _base_class(__l, __comp, __a) {}

    /**
     *  @brief  Builds a %Map from a range.
     *  @param  __first  An input iterator.
     *  @param  __last  An input iterator.
     *
     *  Create a %Map consisting of copies of the elements from
     *  [__first,__last).  This is linear in N if the range is
     *  already sorted, and NlogN otherwise (where N is
     *  distance(__first,__last)).
     */
    template<typename _InputIterator>
    Map(_InputIterator __first, _InputIterator __last)
        : _base_class(allocator_type(MemoryTag::Map)) {
        insert(__first, __last);
    }

    /**
     *  @brief  Builds a %Map from a range.
     *  @param  __first  An input iterator.
     *  @param  __last  An input iterator.
     *  @param  __comp  A comparison functor.
     *  @param  __a  An allocator object.
     *
     *  Create a %Map consisting of copies of the elements from
     *  [__first,__last).  This is linear in N if the range is
     *  already sorted, and NlogN otherwise (where N is
     *  distance(__first,__last)).
     */
    template<typename _InputIterator>
    Map(_InputIterator        __first,
        _InputIterator        __last,
        const _Compare&       __comp,
        const allocator_type& __a = allocator_type(MemoryTag::Map))
        : _base_class(__first, __last, __comp, __a) {}
};
