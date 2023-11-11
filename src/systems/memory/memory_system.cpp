#include "systems/memory/memory_system.hpp"

#include "component/transform.hpp"
#include "resources/material.hpp"
#include "systems/input/control.hpp"

namespace ENGINE_NAMESPACE {

#define MEMORY_SYS_LOG "MemorySystem :: "

static_assert(
    sizeof(MemoryTagType) <= MEMORY_PADDING || MEMORY_PADDING >= 8,
    "Memory padding must be at least 8."
);
static_assert(
    (uint64) MemoryTag::MAX_TAGS <
        ((uint64) 1 << (sizeof(MemoryTagType) * 8 - 4)),
    "No enough space to store all tags. (NOTE: Last 4 bits of enum binary "
    "representation are used to recognize custom allocation)"
);

MemorySystem::MemoryMap MemorySystem::_memory_map = {};
Allocator**             MemorySystem::_allocator_array =
    MemorySystem::initialize_allocator_array(MemorySystem::_memory_map);

// //////////////////////////// //
// MEMORY SYSTEM PUBLIC METHODS //
// //////////////////////////// //

void* MemorySystem::allocate(uint64 size, const MemoryTag tag) {
    auto allocator = _allocator_array[(MemoryTagType) tag];
    return allocator->allocate(size, MEMORY_PADDING);
}
void MemorySystem::deallocate(void* ptr, const MemoryTag tag) {
    auto allocator = _allocator_array[(MemoryTagType) tag];
    if (!allocator->owns(ptr)) {
        std::cout << MEMORY_SYS_LOG << "Wrong memory tag." << std::endl;
        exit(EXIT_FAILURE);
    }
    allocator->free(ptr);
}

void MemorySystem::reset_memory(const MemoryTag tag) {
    auto allocator = _allocator_array[(MemoryTagType) tag];
    allocator->reset();
}

#define convert_to_unit(u)                                                     \
    if (total >= 1024) {                                                       \
        total /= 1024;                                                         \
        used /= 1024;                                                          \
        peek /= 1024;                                                          \
        unit = #u;                                                             \
    }

void MemorySystem::print_usage(const MemoryTag tag) {
    auto allocator = _allocator_array[(MemoryTagType) tag];

    // Calculate total use
    float64 used  = allocator->used();
    float64 total = allocator->total_size();
    float64 peek  = allocator->peak();
    float64 ratio = used / total;

    // Compute unit
    std::string unit = "bytes";
    convert_to_unit(KB);
    convert_to_unit(MB);
    convert_to_unit(GB);
    convert_to_unit(TB);

    // Print
    std::cout << "========================" << std::endl;
    std::cout << used << unit << " / " << total << unit << std::endl;
    std::cout << ratio * 100 << "% / 100%" << std::endl;
    std::cout << "peek : " << peek << unit << std::endl;
    std::cout << "========================" << std::endl;
}

MemoryTag MemorySystem::get_owner(void* p) {
    const auto address = (uint64) p;

    // Get tag
    const auto tag = _memory_map.get_first_before(address);
    if (tag == MemoryTag::MAX_TAGS) return tag;

    // Check if tag owns this memory
    if (!_allocator_array[(MemoryTagType) tag]->owns(p))
        return MemoryTag::MAX_TAGS;

    return tag;
}

// ///////////////////////////// //
// MEMORY SYSTEM PRIVATE METHODS //
// ///////////////////////////// //

// Allocator initializations
#define cal(name)                                                              \
    auto name = new CAllocator();                                              \
    name->init();
#define sal(name, size)                                                        \
    auto name = new StackAllocator(size);                                      \
    name->init();
#define fal(name, size)                                                        \
    auto name = new FreeListAllocator(                                         \
        size, FreeListAllocator::PlacementPolicy::FindFirst                    \
    );                                                                         \
    name->init();
#define lal(name, size)                                                        \
    auto name = new LinearAllocator(size);                                     \
    name->init();

#define pal(name, type, count)                                                 \
    uint64 name##_size = get_aligned(sizeof(type), MEMORY_PADDING);            \
    auto   name        = new PoolAllocator(count * name##_size, name##_size);  \
    name->init();

#define assign_allocator(tag, allocator)                                       \
    allocator_array[(MemoryTagType) MemoryTag::tag] = allocator;               \
    memory_map[allocator->start()]                  = MemoryTag::tag

Allocator** MemorySystem::initialize_allocator_array(MemoryMap& memory_map) {
    const auto allocator_array =
        new Allocator*[(MemoryTagType) MemoryTag::MAX_TAGS]();

    // Define used allocators
    cal(unknown_allocator);
    sal(temp_allocator, MB);
    fal(general_allocator, 128 * MB);
    fal(gpu_data_allocator, MB);
    fal(resource_allocator, MB);
    fal(geom_allocator, 128 * MB);
    lal(init_allocator, MB);
    lal(permanent_allocator, MB);

    // Pools
    pal(texture_pool, Texture, 1024);
    pal(texture_map_pool, TextureMap, 1024);
    pal(material_pool, Material, 1024);
    pal(control_pool, Control, 256);
    pal(transform_pool, Transform, 256);

    // Assign allocators
    assign_allocator(Unknown, unknown_allocator);
    assign_allocator(Temp, temp_allocator);

    assign_allocator(Array, general_allocator);
    assign_allocator(List, general_allocator);
    assign_allocator(Map, general_allocator);
    assign_allocator(Set, general_allocator);
    assign_allocator(String, general_allocator);
    assign_allocator(Callback, general_allocator);
    // Engine
    assign_allocator(Application, init_allocator);
    assign_allocator(Surface, init_allocator);
    assign_allocator(System, init_allocator);
    assign_allocator(Renderer, init_allocator);
    // GPU local
    assign_allocator(GPUTexture, gpu_data_allocator);
    assign_allocator(GPUBuffer, gpu_data_allocator);
    // Resources
    assign_allocator(Resource, resource_allocator);
    assign_allocator(Texture, texture_pool);
    assign_allocator(TextureMap, texture_map_pool);
    assign_allocator(MaterialInstance, material_pool);
    assign_allocator(Geometry, geom_allocator);
    assign_allocator(Shader, permanent_allocator);
    assign_allocator(RenderView, permanent_allocator);
    // Game
    assign_allocator(Game, init_allocator);
    assign_allocator(Control, control_pool);
    assign_allocator(Job, unknown_allocator);
    assign_allocator(Transform, transform_pool);
    assign_allocator(Entity, unknown_allocator);
    assign_allocator(EntityNode, unknown_allocator);
    assign_allocator(Scene, unknown_allocator);

    return allocator_array;
}

// -----------------------------------------------------------------------------
// Memory map
// -----------------------------------------------------------------------------

MemoryTag MemorySystem::MemoryMap::get_first_before(const uint64 address) {
    // Check for nullptr
    if (address == 0) return MemoryTag::MAX_TAGS;

    // Check if alive
    if (_dying) return MemoryTag::MAX_TAGS;

    // Find adequate memory tag
    auto it = lower_bound(address);
    if (it == end()) it--;
    else if (it->first != address) {
        // This is before begin?
        if (it == begin()) return MemoryTag::MAX_TAGS;
        it--;
    }
    const auto tag = it->second;

    return tag;
}

} // namespace ENGINE_NAMESPACE

using namespace ENGINE_NAMESPACE;

// New
void* operator new(std::size_t size, MemoryTag tag) {
    return MemorySystem::allocate(size, tag);
}
void* operator new[](std::size_t size, const MemoryTag tag) {
    return operator new(size, tag);
}

// Delete
void operator delete(void* p) noexcept {
    const auto tag = MemorySystem::get_owner(p);
    if (tag != MemoryTag::MAX_TAGS) MemorySystem::deallocate(p, tag);
    else free(p);
}
void operator delete[](void* p) noexcept { ::operator delete(p); }