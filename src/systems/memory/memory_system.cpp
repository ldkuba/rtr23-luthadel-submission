#include "systems/memory/memory_system.hpp"

#include "resources/material.hpp"

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

Allocator** MemorySystem::_allocator_map =
    MemorySystem::initialize_allocator_map();

// //////////////////////////// //
// MEMORY SYSTEM PUBLIC METHODS //
// //////////////////////////// //

void* MemorySystem::allocate(uint64 size, const MemoryTag tag) {
    auto allocator = _allocator_map[(MemoryTagType) tag];
    return allocator->allocate(size, MEMORY_PADDING);
}
void MemorySystem::deallocate(void* ptr, const MemoryTag tag) {
    auto allocator = _allocator_map[(MemoryTagType) tag];
    if (!allocator->owns(ptr)) {
        std::cout << MEMORY_SYS_LOG << "Wrong memory tag." << std::endl;
        exit(EXIT_FAILURE);
    }
    allocator->free(ptr);
}

void MemorySystem::reset_memory(const MemoryTag tag) {
    auto allocator = _allocator_map[(MemoryTagType) tag];
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
    auto allocator = _allocator_map[(MemoryTagType) tag];

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
#define pal(name, size, chunk_size)                                            \
    auto name = new PoolAllocator(size, chunk_size);                           \
    name->init();

Allocator** MemorySystem::initialize_allocator_map() {
    Allocator** allocator_map =
        new Allocator*[(MemoryTagType) MemoryTag::MAX_TAGS]();

    // Define used allocators
    cal(unknown_allocator);
    sal(temp_allocator, MB);
    fal(general_allocator, 128 * MB);
    fal(gpu_data_allocator, MB);
    fal(resource_allocator, MB);
    fal(geom_allocator, 128 * MB);
    lal(init_allocator, MB);

    // Pools
    uint64 max_texture_count  = 1024;
    uint64 max_material_count = 1024;

    uint64 texture_size  = sizeof(Texture) + MEMORY_PADDING;
    uint64 material_size = sizeof(Material) + MEMORY_PADDING;

    pal(texture_pool, max_texture_count * texture_size, texture_size);
    pal(material_pool, max_material_count * material_size, material_size);

    // Assign allocators
    allocator_map[(MemoryTagType) MemoryTag::Unknown] = unknown_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Temp]    = temp_allocator;

    allocator_map[(MemoryTagType) MemoryTag::Array]       = general_allocator;
    allocator_map[(MemoryTagType) MemoryTag::List]        = general_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Map]         = general_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Set]         = general_allocator;
    allocator_map[(MemoryTagType) MemoryTag::String]      = general_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Callback]    = general_allocator;
    // Engine
    allocator_map[(MemoryTagType) MemoryTag::Application] = init_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Surface]     = init_allocator;
    allocator_map[(MemoryTagType) MemoryTag::System]      = init_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Renderer]    = init_allocator;
    // GPU local
    allocator_map[(MemoryTagType) MemoryTag::GPUTexture]  = gpu_data_allocator;
    allocator_map[(MemoryTagType) MemoryTag::GPUBuffer]   = gpu_data_allocator;
    // Resources
    allocator_map[(MemoryTagType) MemoryTag::Resource]    = resource_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Texture]     = texture_pool;
    allocator_map[(MemoryTagType) MemoryTag::MaterialInstance] = material_pool;
    allocator_map[(MemoryTagType) MemoryTag::Geometry]         = geom_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Shader]     = resource_allocator;
    // Game
    allocator_map[(MemoryTagType) MemoryTag::Game]       = init_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Job]        = unknown_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Transform]  = unknown_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Entity]     = unknown_allocator;
    allocator_map[(MemoryTagType) MemoryTag::EntityNode] = unknown_allocator;
    allocator_map[(MemoryTagType) MemoryTag::Scene]      = unknown_allocator;

    return allocator_map;
}

} // namespace ENGINE_NAMESPACE

using namespace ENGINE_NAMESPACE;

// New
#if OLD_IMPLEMENTATION == 0
void* operator new(std::size_t size) {
    void* full_ptr = malloc(size + MEMORY_PADDING);
    void* data_ptr = (void*) ((uint64) full_ptr + MEMORY_PADDING);

    MemoryTag* header_ptr = (MemoryTag*) full_ptr;
    *header_ptr           = (MemoryTag) 0;

    return data_ptr;
}
#endif
void* operator new(std::size_t size, MemoryTag tag) {
#if OLD_IMPLEMENTATION
    void* full_ptr =
        ENGINE_NAMESPACE::MemorySystem::allocate(size + MEMORY_PADDING, tag);
    void* data_ptr = (void*) ((uint64) full_ptr + MEMORY_PADDING);

    MemoryTagType tag_data = ((MemoryTagType) tag << 4) | 0b1111;

    MemoryTag* header_ptr = (MemoryTag*) full_ptr;
    *header_ptr           = (MemoryTag) tag_data;

    return data_ptr;
#else
    void* full_ptr =
        ENGINE_NAMESPACE::MemorySystem::allocate(size + MEMORY_PADDING, tag);
    void* data_ptr = (void*) ((uint64) full_ptr + MEMORY_PADDING);

    MemoryTag* header_ptr = (MemoryTag*) full_ptr;
    *header_ptr           = (MemoryTag) tag;

    return data_ptr;
#endif
}

#if OLD_IMPLEMENTATION == 0
void* operator new[](std::size_t size) { return operator new(size); }
#endif
void* operator new[](std::size_t size, const MemoryTag tag) {
    return operator new(size, tag);
}

void log_bytes(const void* const p, const size_t size) {
    for (size_t i = 0; i < size; i++) {
        const auto byte = (ubyte*) ((size_t) p + (size - 1 - i));
        for (size_t j = 0; j < 8; j++) {
            const auto bit = (*byte >> (7 - j)) & 0b1;
            if (bit == 1) //
                std::cout << "1";
            else std::cout << "0";
        }
        std::cout << " ";
    }
    std::cout << std::endl;
}

// Delete
void operator delete(void* p) noexcept {
#if OLD_IMPLEMENTATION
    if (p == nullptr) return;
    void*         full_ptr   = (void*) ((uint64) p - MEMORY_PADDING);
    MemoryTag*    header_ptr = (MemoryTag*) full_ptr;
    MemoryTagType tag_data   = (MemoryTagType) *header_ptr;

    log_bytes(full_ptr, MEMORY_PADDING);

    MemoryTag     tag      = (MemoryTag) (tag_data >> 4);
    MemoryTagType tag_type = tag_data & 0b1111;

    // 240 = 11110000
    if (tag_type != 0b1111) free(p);
    else MemorySystem::deallocate(full_ptr, tag);
#else
    if (p == nullptr) return;
    void*          full_ptr = (void*) ((uint64) p - MEMORY_PADDING);
    MemoryTagType* tag_ptr  = (MemoryTagType*) full_ptr;
    MemoryTag      tag      = (MemoryTag) *tag_ptr;

    // 240 = 11110000
    if (tag == MemoryTag::Unknown) free(full_ptr);
    else MemorySystem::deallocate(p, tag);
#endif
}