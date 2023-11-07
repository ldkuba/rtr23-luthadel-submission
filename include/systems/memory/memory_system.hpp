#pragma once

#include "memory_allocators/c_allocator.hpp"
#include "memory_allocators/linear_allocator.hpp"
#include "memory_allocators/stack_allocator.hpp"
#include "memory_allocators/pool_allocator.hpp"
#include "memory_allocators/free_list_allocator.hpp"

#include <iostream>
#include <map>
#include <type_traits>
#include <memory>

namespace ENGINE_NAMESPACE {

typedef uint16 MemoryTagType;
#define MEMORY_PADDING 8

// Size reference points
#define KB 1024
#define MB KB * 1024
#define GB MB * 1024

/**
 * @brief Memory tag used for allocation. Indicates what custom allocator, if
 * any, should be used for given allocation. Usually used with @a new.
 */
enum class MemoryTag : MemoryTagType {
    // For temporary use. Should be assigned one of the below or have a new
    // tag
    // created.
    Unknown,
    Temp,
    // Data types
    Array,
    List,
    Map,
    Set,
    String,
    Callback,
    // === Engine allocations ===
    Application,
    Surface,
    System,
    Renderer,
    // Renderer
    GPUTexture,
    GPUBuffer,
    // Resources
    Resource,
    Texture,
    MaterialInstance,
    Geometry,
    Shader,
    // === Game allocations ===
    Game,
    Job,
    Transform,
    Entity,
    EntityNode,
    Scene,

    MAX_TAGS
};

/**
 * @brief Memory system. Responsible for memory management and tracking, custom
 * allocators, allocations and deallocations for the engine. Collection of
 * static methods, cant be instantiated.
 */
class MemorySystem {
  public:
    MemorySystem()  = delete;
    ~MemorySystem() = delete;

    /**
     * @brief Allocates memory chunk with a custom allocator
     * @param size Chunk size in bytes
     * @param tag Memory tag of allocator to be used
     * @return void* Reference to allocated memory
     */
    static void* allocate(uint64 size, const MemoryTag tag);
    /**
     * @brief Deallocate memory chunk with a custom allocator
     * @param ptr Reference to the first byte of the chunk
     * @param tag Memory tag of allocator to be used
     */
    static void  deallocate(void* ptr, const MemoryTag tag);
    /**
     * @brief Clears all memory for allocator with a given tag
     * @param tag Memory tag of targeted allocator
     */
    static void  reset_memory(const MemoryTag tag);
    /**
     * @brief Output usage for allocator with a given tag
     * @param tag Memory tag of targeted allocator
     */
    static void  print_usage(const MemoryTag tag);

    /**
     * @brief Get owner of a given address
     * @param ptr Address of an allocated object
     * @return MemoryTag Tag of the owning allocator
     */
    static MemoryTag get_owner(void* ptr);

  private:
    class MemoryMap : public std::map<uint64, MemoryTag> {
      public:
        using std::map<uint64, MemoryTag>::map;

        ~MemoryMap() { _dying = true; }

        MemoryTag get_first_before(const uint64 address);

      private:
        bool _dying = false;
    };

    static MemoryMap   _memory_map;
    static Allocator** _allocator_array;

    static Allocator** initialize_allocator_array(MemoryMap& memory_map);
};

} // namespace ENGINE_NAMESPACE

// New
void* operator new(std::size_t size, const ENGINE_NAMESPACE::MemoryTag tag);
void* operator new[](std::size_t size, const ENGINE_NAMESPACE::MemoryTag tag);

// Delete
void operator delete(void* p) noexcept;
void operator delete[](void* p) noexcept;

// New delete operator
template<typename T>
inline void del(T* p) {
    delete p;
}

// -----------------------------------------------------------------------------
// Typed allocator
// -----------------------------------------------------------------------------

namespace ENGINE_NAMESPACE {

template<class T>
struct TAllocator {
    MemoryTag tag;

    typedef T value_type;
    // default ctor not required by C++ Standard Library
    TAllocator(MemoryTag tag = MemoryTag::Unknown) noexcept : tag(tag) {}

    // A converting copy constructor:
    template<class U>
    TAllocator(const TAllocator<U>& other) noexcept {
        tag = other.tag;
    }
    template<class U>
    bool operator==(const TAllocator<U>&) const noexcept {
        return true;
    }
    template<class U>
    bool operator!=(const TAllocator<U>&) const noexcept {
        return false;
    }
    T*   allocate(const std::size_t n) const;
    void deallocate(T* const p, std::size_t n) const noexcept;
};

template<class T>
T* TAllocator<T>::allocate(const std::size_t n) const {
    return (T*) operator new(n * sizeof(T), this->tag);
}
template<class T>
void TAllocator<T>::deallocate(T* const p, std::size_t n) const noexcept {
    ::operator delete(p);
}

} // namespace ENGINE_NAMESPACE

// Make
namespace std {
/// std::make_unique for single objects
template<typename _Tp, typename... _Args>
inline unique_ptr<_Tp> make_unique(
    ENGINE_NAMESPACE::MemoryTag tag, _Args&&... __args
) {
    return unique_ptr<_Tp>(new (tag) _Tp(std::forward<_Args>(__args)...));
}

/// std::make_unique for arrays of unknown bound
template<typename _Tp>
inline unique_ptr<_Tp[]> make_unique(
    ENGINE_NAMESPACE::MemoryTag tag, size_t __num
) {
    return unique_ptr<_Tp>(new (tag) remove_extent_t<_Tp>[__num]());
}

/**
 *  @brief  Create an object that is owned by a shared_ptr.
 *  @param  tag  Memory tag
 *  @param  __args  Arguments for the @a _Tp object's constructor.
 *  @return A shared_ptr that owns the newly created object.
 *  @throw  std::bad_alloc, or an exception thrown from the
 *          constructor of @a _Tp.
 */
template<typename _Tp, typename... _Args>
inline shared_ptr<_Tp> make_shared(
    ENGINE_NAMESPACE::MemoryTag tag, _Args&&... __args
) {
    typedef typename std::remove_cv<_Tp>::type _Tp_nc;
    return std::allocate_shared<_Tp>(
        ENGINE_NAMESPACE::TAllocator<_Tp_nc>(tag),
        std::forward<_Args>(__args)...
    );
}
} // namespace std