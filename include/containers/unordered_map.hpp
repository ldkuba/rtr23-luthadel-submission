#pragma once

#include <unordered_map>

#include "systems/memory/memory_system.hpp"

namespace ENGINE_NAMESPACE {

/**
 *  @brief A standard container composed of unique keys (containing
 *  at most one of each key value) that associates values of another type
 *  with the keys.
 *
 *  @ingroup unordered_associative_containers
 *
 *  @tparam  _Key    Type of key objects.
 *  @tparam  _Tp     Type of mapped objects.
 *  @tparam  _Hash   Hashing function object type, defaults to hash<_Value>.
 *  @tparam  _Pred   Predicate function object type, defaults
 *                   to equal_to<_Value>.
 *
 *  Meets the requirements of a <a href="tables.html#65">container</a>, and
 *  <a href="tables.html#xx">unordered associative container</a>
 *
 * The resulting value type of the container is std::pair<const _Key, _Tp>.
 *
 *  Base is _Hashtable, dispatched at compile time via template
 *  alias __umap_hashtable.
 */
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
    typedef std::unordered_map<_Key, _Tp, _Hash, _Pred> default_version;
    typedef typename _base_class::hasher                hasher;
    typedef typename _base_class::key_equal             key_equal;

  public:
    using std::unordered_map<_Key, _Tp, _Hash, _Pred, allocator_type>::
        unordered_map;
    using std::unordered_map<_Key, _Tp, _Hash, _Pred, allocator_type>::insert;

    /// Default constructor.
    UnorderedMap() : _base_class(allocator_type(MemoryTag::Map)) {}

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

    /// Copy constructor.
    UnorderedMap(const default_version& __x)
        : _base_class(allocator_type(MemoryTag::Map)) {
        for (auto var : __x) {
            insert(var);
        }
    }

    /// Move constructor.
    UnorderedMap(default_version&& __x)
        : _base_class(allocator_type(MemoryTag::Map)) {
        for (auto var : __x) {
            insert(var);
        }
    }

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

  public: // Additions

    /**
     * @brief Check whether the %UnorderedMap contains a given key.
     *
     * @param __key Key to search for.
     * @returns true if %UnorderedMap contains the key; false otherwise.
     */
    bool contains(const _Key& __key) const {
        const auto& i = this->find(__key);
        return i != this->end();
    }
    // Move version
    bool contains(_Key&& __key) const {
        const auto& i = this->find(__key);
        return i != this->end();
    }
};

} // namespace ENGINE_NAMESPACE