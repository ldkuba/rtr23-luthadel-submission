#include "systems/memory/memory_allocators/gpu_free_list_allocator.hpp"

#include "logger.hpp"

#include <algorithm> // std::max

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
GPUFreeListAllocator::GPUFreeListAllocator(
    const uint64          totalSize,
    const uint64          begin_offset,
    const PlacementPolicy pPolicy
)
    : Allocator(totalSize) {
    _placement_policy = pPolicy;
    _start_ptr        = (void*) begin_offset;
}

// ////////////////////////////////// //
// FREE LIST ALLOCATOR PUBLIC METHODS //
// ////////////////////////////////// //

void GPUFreeListAllocator::init() { this->reset(); }

void* GPUFreeListAllocator::allocate(
    const uint64 size, const uint64 alignment
) {
#ifdef WARN_FREE_LIST_SIZE_FOR_GPU
    if (size < sizeof(Node))
        Logger::warning(
            ALLOCATOR_LOG,
            "Free list allocation size should be bigger then or equal to ",
            sizeof(Node),
            ", which is sizeof(Node)."
        );
#endif // DEBUG
    if (alignment < 4)
        Logger::fatal(
            ALLOCATOR_LOG, "Free list allocation alignment must be at least 4."
        );

    // Search through the free list for a free block that has enough space to
    // allocate our data
    uint64 padding;
    Node   previous_node, affected_node;
    find(size, alignment, padding, previous_node, affected_node);

    if (affected_node == _free_list.end())
        Logger::fatal(
            ALLOCATOR_LOG, "Free list allocator out of memory error."
        );

    const uint64 offset        = affected_node->offset;
    const uint64 required_size = size + padding;
    const uint64 rest          = affected_node->block_size - required_size;

    if (rest > 0) {
        // We have to split the block into the data block and a free block of
        // size 'rest'
        FreeHeader new_free_node {};
        new_free_node.block_size = rest;
        new_free_node.offset     = affected_node->offset + required_size;
        _free_list.insert_after(affected_node, new_free_node);
    }
    _free_list.erase_after(previous_node);

    // Setup data block
    const uint64 data_address = offset + padding;
    _allocated[data_address]  = { required_size, (uint8) padding };

    // Debug vars
    _used += required_size;
    _peak = std::max(_peak, _used);

    return (void*) data_address;
}

void GPUFreeListAllocator::free(void* ptr) {
    const uint64           offset            = (uint64) ptr;
    const AllocationHeader allocation_header = _allocated[offset];
    _allocated.erase(offset);

    FreeHeader free_node {};
    free_node.block_size = allocation_header.block_size;
    free_node.offset     = offset - allocation_header.padding;

    auto prev = _free_list.before_begin();
    for (auto curr = _free_list.begin(); curr != _free_list.end(); curr++) {
        if (offset < curr->offset) break;
        prev = curr;
    }
    _free_list.insert_after(prev, free_node);

    _used -= free_node.block_size;

    // Merge contiguous nodes
    coalescence(prev, std::next(prev));
}

void GPUFreeListAllocator::reset() {
    _used = 0;
    _peak = 0;
    _free_list.clear();
    _allocated.clear();
    FreeHeader free_node {};
    free_node.block_size = _total_size;
    free_node.offset     = (uint64) _start_ptr;
    _free_list.emplace_front(free_node);
}

bool GPUFreeListAllocator::allocated(const void* ptr, const uint64 size) {
    // Get iterator to the closest allocated address <= to ptr
    auto it = _allocated.upper_bound((uint64) ptr);
    it--;

    // Check if ptr points to an address before the first allocated address
    if (it == _allocated.end()) return false;
    // if this iterator envelops the whole requested region we are good
    return it->first + it->second.block_size >= (uint64) ptr + size;
}

// /////////////////////////////////// //
// FREE LIST ALLOCATOR PRIVATE METHODS //
// /////////////////////////////////// //

void GPUFreeListAllocator::find(
    const uint64 size,
    const uint64 alignment,
    uint64&      padding,
    Node&        previous_node,
    Node&        found_node
) {
    switch (_placement_policy) {
    case FindFirst:
        find_first(size, alignment, padding, previous_node, found_node);
        break;
    case FindBest:
        find_best(size, alignment, padding, previous_node, found_node);
        break;
    }
}

void GPUFreeListAllocator::find_first(
    const uint64 size,
    const uint64 alignment,
    uint64&      padding,
    Node&        previous_node,
    Node&        found_node
) {
    // Iterate list and return the first free block with a size >= than given
    // size
    previous_node = _free_list.before_begin();
    for (found_node = _free_list.begin(); found_node != _free_list.end();
         found_node++) {
        padding = calculate_padding(found_node->offset, alignment);
        const auto required_space = size + padding;
        if (found_node->block_size >= required_space) return;
        previous_node = found_node;
    }
}

void GPUFreeListAllocator::find_best(
    const uint64 size,
    const uint64 alignment,
    uint64&      padding,
    Node&        previous_node,
    Node&        found_node
) {
    return;
    // TODO: IMPLEMENT
    // // Iterate WHOLE list keeping a pointer to the best fit
    // uint64 smallest_diff = uint64_max;
    // Node*  best_block    = nullptr;

    // Node *it = _free_list.head, *it_prev = nullptr;
    // while (it != nullptr) {
    //     padding = calculate_padding_with_header(
    //         (uint64) it, alignment, sizeof(AllocationHeader)
    //     );
    //     const uint64 required_space = size + padding;
    //     if (it->data.block_size >= required_space &&
    //         (it->data.block_size - required_space < smallest_diff)) {
    //         best_block = it;
    //     }
    //     it_prev = it;
    //     it      = it->next;
    // }
    // previous_node = it_prev;
    // found_node    = best_block;
}

void GPUFreeListAllocator::coalescence(Node previous_node, Node free_node) {
    Node next_node = std::next(free_node);
    if (next_node != _free_list.end() &&
        free_node->offset + free_node->block_size == next_node->offset) {
        free_node->block_size += next_node->block_size;
        _free_list.erase_after(free_node);
    }
    if (previous_node != _free_list.before_begin() &&
        previous_node->offset + previous_node->block_size ==
            free_node->offset) {
        previous_node->block_size += free_node->block_size;
        _free_list.erase_after(previous_node);
    }
}

} // namespace ENGINE_NAMESPACE