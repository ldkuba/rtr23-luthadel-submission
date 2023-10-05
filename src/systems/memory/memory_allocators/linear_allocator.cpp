#include "systems/memory/memory_allocators/linear_allocator.hpp"

#include "logger.hpp"

#include <algorithm> // max

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
LinearAllocator::LinearAllocator(const uint64 total_size)
    : Allocator(total_size) {}

// /////////////////////////////// //
// LINEAR ALLOCATOR PUBLIC METHODS //
// /////////////////////////////// //

void* LinearAllocator::allocate(const uint64 size, const uint64 alignment) {
    uint64       padding         = 0;
    const uint64 current_address = (uint64) _start_ptr + _offset;

    // If alignment is required:
    // Find the next aligned memory address and update offset
    if (alignment != 0 && _offset % alignment != 0)
        padding = calculate_padding(current_address, alignment);

    if (_offset + padding + size > _total_size)
        Logger::fatal(ALLOCATOR_LOG, "Linear allocator out of memory error.");

    _offset += padding + size; // Apply padding & Move by size
    const uint64 next_address = current_address + padding;

    // Debug data
    _used = _offset;
    _peak = std::max(_peak, _used);

    return (void*) next_address;
}

void LinearAllocator::free(void* ptr) {
    return;
    Logger::fatal(
        ALLOCATOR_LOG,
        "Cant free() linear allocator. Use reset() method instead."
    );
}

void LinearAllocator::reset() {
    _offset = 0;
    _used   = 0;
    _peak   = 0;
}

} // namespace ENGINE_NAMESPACE