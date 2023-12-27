#include "systems/memory/memory_allocators/stack_allocator.hpp"

#include "logger.hpp"

#include <algorithm> /* max */

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
StackAllocator::StackAllocator(const uint64 total_size)
    : Allocator(total_size) {}

// ////////////////////////////// //
// STACK ALLOCATOR PUBLIC METHODS //
// ////////////////////////////// //

void* StackAllocator::allocate(const uint64 size, const uint64 alignment) {
    const uint64 current_address = (uint64) _start_ptr + _offset;

    uint64 padding = calculate_padding_with_header(
        current_address, alignment, sizeof(AllocationHeader)
    );

    if (_offset + padding + size > _total_size)
        Logger::fatal(ALLOCATOR_LOG, "Stack allocator out of memory error.");

    _offset += padding;

    const uint64      next_address   = current_address + padding;
    const uint64      header_address = next_address - sizeof(AllocationHeader);
    AllocationHeader  allocation_header { (uint8) padding };
    AllocationHeader* header_ptr = (AllocationHeader*) header_address;
    *header_ptr                  = allocation_header;

    _offset += size;

    // Debug variables
    _used = _offset;
    _peak = std::max(_peak, _used);

    return (void*) next_address;
}

void StackAllocator::free(void* ptr) {
    // Move offset back to clear address
    const uint64 current_address = (uint64) ptr;
    const uint64 header_address  = current_address - sizeof(AllocationHeader);
    const AllocationHeader* allocation_header =
        (AllocationHeader*) header_address;

    const auto object_offset = current_address - (uint64) _start_ptr;
    if (object_offset > _offset) {
        Logger::warning(
            ALLOCATOR_LOG,
            "Stack allocator deallocation order is disturbed. Make sure stack "
            "deallocations happen in reverse order of their allocations."
        );
        return;
    }

    _offset = object_offset - allocation_header->padding;
    _used   = _offset;
}

void StackAllocator::reset() {
    _offset = 0;
    _used   = 0;
    _peak   = 0;
}

} // namespace ENGINE_NAMESPACE