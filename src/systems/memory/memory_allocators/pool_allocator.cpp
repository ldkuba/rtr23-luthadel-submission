#include "systems/memory/memory_allocators/pool_allocator.hpp"

#include "logger.hpp"

#include <algorithm> //max

// Constructor & Destructor
PoolAllocator::PoolAllocator(const uint64 total_size, const uint64 chunk_size)
    : Allocator(total_size) {
    if (chunk_size < 8)
        Logger::fatal(
            ALLOCATOR_LOG,
            "Pool allocator's chunk size must be greater or equal to 8."
        );
    if (total_size % chunk_size != 0)
        Logger::fatal(
            ALLOCATOR_LOG,
            "Pool allocator's total size must be a multiple of Chunk Size."
        );

    this->_chunk_size = chunk_size;
}

// ///////////////////////////// //
// POOL ALLOCATOR PUBLIC METHODS //
// ///////////////////////////// //

void* PoolAllocator::allocate(
    const uint64 allocation_size, const uint64 alignment
) {
    if (allocation_size != this->_chunk_size)
        Logger::fatal(
            ALLOCATOR_LOG,
            "Allocation size for pool allocator must be equal to chunk size."
        );

    Node* free_position = _free_list.pop();

    if (free_position == nullptr)
        Logger::fatal(ALLOCATOR_LOG, "The pool allocator is full");

    // Debug info
    _used += _chunk_size;
    _peak = std::max(_peak, _used);

    return (void*) free_position;
}

void PoolAllocator::free(void* ptr) {
    _used -= _chunk_size;
    _free_list.push((Node*) ptr);
}

void PoolAllocator::reset() {
    _used = 0;
    _peak = 0;

    // Create a linked-list with all free positions
    const int chunk_count = _total_size / _chunk_size;
    for (int i = 0; i < chunk_count; ++i) {
        uint64 address = (uint64) _start_ptr + i * _chunk_size;
        _free_list.push((Node*) address);
    }
}