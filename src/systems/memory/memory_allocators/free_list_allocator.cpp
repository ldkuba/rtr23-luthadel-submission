#include "systems/memory/memory_allocators/free_list_allocator.hpp"

#include "logger.hpp"

#include <algorithm> // std::max

namespace ENGINE_NAMESPACE {

// Constructor & Destructor
FreeListAllocator::FreeListAllocator(
    const uint64 totalSize, const PlacementPolicy pPolicy
)
    : Allocator(totalSize) {
    _placement_policy = pPolicy;
}

// ////////////////////////////////// //
// FREE LIST ALLOCATOR PUBLIC METHODS //
// ////////////////////////////////// //

#define FREE_LIST_ALLOCATION_SIZE_WAR 0

void* FreeListAllocator::allocate(const uint64 size, const uint64 alignment) {
    if (size < sizeof(Node))
#if FREE_LIST_ALLOCATION_SIZE_WAR == 1
        Logger::warning(
            ALLOCATOR_LOG,
            "Free list allocation size should be bigger then or equal to ",
            sizeof(Node),
            ", which is sizeof(Node)."
        );
#endif
    if (alignment < 4)
        Logger::fatal(
            ALLOCATOR_LOG, "Free list allocation alignment must be at least 4."
        );

    // Search through the free list for a free block that has enough space to
    // allocate our data
    uint64 padding;
    Node * affected_node, *previous_node;
    find(size, alignment, padding, previous_node, affected_node);

    if (affected_node == nullptr)
        Logger::fatal(
            ALLOCATOR_LOG, "Free list allocator out of memory error."
        );

    const uint64 allocation_header_size = sizeof(AllocationHeader);
    const uint64 alignment_padding      = padding - allocation_header_size;
    uint64       required_size          = size + padding;

    const uint64 rest = affected_node->data.block_size - required_size;

    if (rest >= sizeof(Node)) {
        // We have to split the block into the data block and a free block of
        // size 'rest'
        Node* new_free_node = (Node*) ((uint64) affected_node + required_size);
        new_free_node->data.block_size = rest;
        _free_list.insert(affected_node, new_free_node);
    } else if (rest > 0) required_size += rest;
    _free_list.remove(previous_node, affected_node);

    // Setup data block
    const uint64 header_address = (uint64) affected_node + alignment_padding;
    const uint64 data_address   = header_address + allocation_header_size;
    ((AllocationHeader*) header_address)->block_size = required_size;
    ((AllocationHeader*) header_address)->padding    = alignment_padding;

    // Debug vars
    _used += required_size;
    _peak = std::max(_peak, _used);

    return (void*) data_address;
}

void FreeListAllocator::free(void* ptr) {
    // Insert it in a sorted position by the address number
    const uint64 current_address = (uint64) ptr;
    const uint64 header_address  = current_address - sizeof(AllocationHeader);
    const AllocationHeader* allocation_header {
        (AllocationHeader*) header_address
    };

    Node* free_node = (Node*) (header_address - allocation_header->padding);
    free_node->data.block_size = allocation_header->block_size;
    free_node->next            = nullptr;

    Node *it = _free_list.head, *it_prev = nullptr;
    while (it != nullptr) {
        if (ptr < it) {
            _free_list.insert(it_prev, free_node);
            break;
        }
        it_prev = it;
        it      = it->next;
    }

    _used -= free_node->data.block_size;

    // Merge contiguous nodes
    coalescence(it_prev, free_node);
}

void FreeListAllocator::reset() {
    _used                       = 0;
    _peak                       = 0;
    Node* first_node            = (Node*) _start_ptr;
    first_node->data.block_size = _total_size;
    first_node->next            = nullptr;
    _free_list.head             = nullptr;
    _free_list.insert(nullptr, first_node);
}

// /////////////////////////////////// //
// FREE LIST ALLOCATOR PRIVATE METHODS //
// /////////////////////////////////// //

void FreeListAllocator::find(
    const uint64 size,
    const uint64 alignment,
    uint64&      padding,
    Node*&       previous_node,
    Node*&       found_node
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

void FreeListAllocator::find_first(
    const uint64 size,
    const uint64 alignment,
    uint64&      padding,
    Node*&       previous_node,
    Node*&       found_node
) {
    // Iterate list and return the first free block with a size >= than given
    // size
    Node *it = _free_list.head, *it_prev = nullptr;
    while (it != nullptr) {
        padding = calculate_padding_with_header(
            (uint64) it, alignment, sizeof(AllocationHeader)
        );
        const uint64 required_space = size + padding;
        if (it->data.block_size >= required_space) { break; }
        it_prev = it;
        it      = it->next;
    }
    previous_node = it_prev;
    found_node    = it;
}

void FreeListAllocator::find_best(
    const uint64 size,
    const uint64 alignment,
    uint64&      padding,
    Node*&       previous_node,
    Node*&       found_node
) {
    // Iterate WHOLE list keeping a pointer to the best fit
    uint64 smallest_diff = uint64_max;
    Node*  best_block    = nullptr;

    Node *it = _free_list.head, *it_prev = nullptr;
    while (it != nullptr) {
        padding = calculate_padding_with_header(
            (uint64) it, alignment, sizeof(AllocationHeader)
        );
        const uint64 required_space = size + padding;
        if (it->data.block_size >= required_space &&
            (it->data.block_size - required_space < smallest_diff)) {
            best_block = it;
        }
        it_prev = it;
        it      = it->next;
    }
    previous_node = it_prev;
    found_node    = best_block;
}

void FreeListAllocator::coalescence(Node* previous_node, Node* free_node) {
    if (free_node->next != nullptr &&
        (uint64) free_node + free_node->data.block_size ==
            (uint64) free_node->next) {
        free_node->data.block_size += free_node->next->data.block_size;
        _free_list.remove(free_node, free_node->next);
    }

    if (previous_node != nullptr &&
        (uint64) previous_node + previous_node->data.block_size ==
            (uint64) free_node) {
        previous_node->data.block_size += free_node->data.block_size;
        _free_list.remove(previous_node, free_node);
    }
}

} // namespace ENGINE_NAMESPACE