#pragma once

#include "memory_allocators/c_allocator.hpp"
#include "memory_allocators/linear_allocator.hpp"
#include "memory_allocators/stack_allocator.hpp"
#include "memory_allocators/pool_allocator.hpp"
#include "memory_allocators/free_list_allocator.hpp"

#include <iostream>
#include <type_traits>
#include <memory>

typedef ENGINE_NAMESPACE::uint16 MemoryTagType;
#define MEMORY_PADDING 8

// Size reference points
#define KB 1024
#define MB KB * 1024
#define GB MB * 1024

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

namespace ENGINE_NAMESPACE {

class MemorySystem {
  public:
    static void* allocate(uint64 size, const MemoryTag tag);
    static void  deallocate(void* ptr, const MemoryTag tag);
    static void  reset_memory(const MemoryTag tag);
    static void  print_usage(const MemoryTag tag);

  private:
    MemorySystem();
    ~MemorySystem();

    static Allocator** _allocator_map;
    static Allocator** initialize_allocator_map();
};

} // namespace ENGINE_NAMESPACE

// New
void* operator new(std::size_t size, const MemoryTag tag);
void* operator new[](std::size_t size, const MemoryTag tag);

// Delete
void operator delete(void* p) noexcept;

namespace ENGINE_NAMESPACE {

// -----------------------------------------------------------------------------
// Typed allocator
// -----------------------------------------------------------------------------
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
void TAllocator<T>::deallocate(T* const p, std::size_t) const noexcept {
    operator delete(p);
}

} // namespace ENGINE_NAMESPACE

// Make
namespace std {
template<typename _Tp, typename... _Args>
inline typename _MakeUniq<_Tp>::__single_object make_unique(
    MemoryTag tag, _Args&&... __args
) {
    return unique_ptr<_Tp>(new (tag) _Tp(std::forward<_Args>(__args)...));
}
template<typename _Tp, typename... _Args>
inline shared_ptr<_Tp> make_shared(MemoryTag tag, _Args&&... __args) {
    typedef typename std::remove_cv<_Tp>::type _Tp_nc;
    return std::allocate_shared<_Tp>(
        ENGINE_NAMESPACE::TAllocator<_Tp_nc>(tag),
        std::forward<_Args>(__args)...
    );
}
} // namespace std