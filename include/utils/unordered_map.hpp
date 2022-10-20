#pragma once

#include <unordered_map>

#include "memory_system.hpp"

template<
    typename _Key,
    typename _Tp,
    typename _Hash = std::hash<_Key>,
    typename _Pred = std::equal_to<_Key>>
class UnorderedMap
    : public std::unordered_map<_Key, _Tp, _Hash, _Pred, TAllocator<_Tp>> {

  private:
    typedef TAllocator<_Tp> allocator_type;
    typedef std::unordered_map<_Key, _Tp, _Hash, _Pred, allocator_type>
        _base_class;
    typedef std::__umap_hashtable<_Key, _Tp, _Hash, _Pred, allocator_type>
                                           _Hashtable;
    typedef typename _Hashtable::hasher    hasher;
    typedef typename _Hashtable::key_equal key_equal;

  public:
    using std::unordered_map<_Key, _Tp, _Hash, _Pred, allocator_type>::
        unordered_map;

    /// Default constructor.
    // UnorderedMap() = default;
    UnorderedMap()
        : _base_class(
              0, hasher(), key_equal(), allocator_type(MemoryTag::Map)
          ) {}

    /**
     *  @brief  Default constructor creates no elements.
     *  @param __n  Minimal initial number of buckets.
     *  @param __hf  A hash functor.
     *  @param __eql  A key equality functor.
     *  @param __a  An allocator object.
     */
    explicit UnorderedMap(
        uint64                __n,
        const hasher&         __hf  = hasher(),
        const key_equal&      __eql = key_equal(),
        const allocator_type& __a   = allocator_type(MemoryTag::Map)
    )
        : _base_class(__n, __hf, __eql, __a) {}

    /**
     *  @brief  Builds an %UnorderedMap from a range.
     *  @param  __first  An input iterator.
     *  @param  __last  An input iterator.
     *  @param __n  Minimal initial number of buckets.
     *  @param __hf  A hash functor.
     *  @param __eql  A key equality functor.
     *  @param __a  An allocator object.
     *
     *  Create an %UnorderedMap consisting of copies of the elements from
     * [__first,__last).  This is linear in N (where N is
     *  distance(__first,__last)).
     */
    template<typename _InputIterator>
    UnorderedMap(
        _InputIterator        __first,
        _InputIterator        __last,
        uint64                __n   = 0,
        const hasher&         __hf  = hasher(),
        const key_equal&      __eql = key_equal(),
        const allocator_type& __a   = allocator_type(MemoryTag::Map)
    )
        : _base_class(__first, __last, __n, __hf, __eql, __a) {}

    /**
     *  @brief  Builds an %UnorderedMap from an initializer_list.
     *  @param  __l  An initializer_list.
     *  @param __n  Minimal initial number of buckets.
     *  @param __hf  A hash functor.
     *  @param __eql  A key equality functor.
     *  @param  __a  An allocator object.
     *
     *  Create an %UnorderedMap consisting of copies of the elements in the
     *  list. This is linear in N (where N is @a __l.size()).
     */
    UnorderedMap(
        std::initializer_list<_Tp> __l,
        uint64                     __n   = 0,
        const hasher&              __hf  = hasher(),
        const key_equal&           __eql = key_equal(),
        const allocator_type&      __a   = allocator_type(MemoryTag::Map)
    )
        : _base_class(__l, __n, __hf, __eql, __a) {}
};
