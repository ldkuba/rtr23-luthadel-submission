#pragma once

#include "memory_allocators/c_allocator.hpp"
#include "memory_allocators/linear_allocator.hpp"
#include "memory_allocators/stack_allocator.hpp"
#include "memory_allocators/pool_allocator.hpp"
#include "memory_allocators/free_list_allocator.hpp"

#include <iostream>
#include <type_traits>
#include <memory>

#define MEMORY_SYS_LOG "MemorySystem :: "

#define MEMORY_TAG_TYPE uint8
#define MEMORY_PADDING 8

enum class MemoryTag : MEMORY_TAG_TYPE {
    // For temporary use. Should be assigned one of the below or have a new tag
    // created.
    Unknown,
    // Data types
    Array,
    List,
    Map,
    String,
    Callback,
    // Engine allocations
    Application,
    System,
    Renderer,
    // Game allocations
    Game,
    Job,
    Texture,
    MaterialInstance,
    Transform,
    Entity,
    EntityNode,
    Scene,
    Resource,
    Vulkan,
    // "External" vulkan allocations, for reporting purposes only.
    // VULKAN_EXT,
    // DIRECT3D,
    // OpenGL,
    // Representation of GPU-local/vram
    // GPU_LOCAL,
    // BITMAP_FONT,
    // SYSTEM_FONT,

    MAX_TAGS
};

class MemorySystem {
  public:

    static void* allocate(uint64 size, const MemoryTag tag) {
        auto allocator = _allocator_map[(MEMORY_TAG_TYPE) tag];
        return allocator->allocate(size, MEMORY_PADDING);
    }

    static void deallocate(void* ptr, const MemoryTag tag) {
        auto allocator = _allocator_map[(MEMORY_TAG_TYPE) tag];
        if (!allocator->owns(ptr)) {
            std::cout << MEMORY_SYS_LOG << "Wrong memory tag." << std::endl;
            exit(EXIT_FAILURE);
        }
        allocator->free(ptr);
    }

  private:
    static Allocator** _allocator_map;
    static Allocator** initialize_allocator_map();

    MemorySystem();
    ~MemorySystem();
};

// New
void* operator new(std::size_t size, const MemoryTag tag);
void* operator new[](std::size_t size, const MemoryTag tag);

// Delete
void operator delete(void* p) noexcept;

// Typed allocator
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
        TAllocator<_Tp_nc>(tag), std::forward<_Args>(__args)...
    );
}
} // namespace std