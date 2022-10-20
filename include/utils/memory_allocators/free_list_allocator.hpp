#pragma once

#include "allocator.hpp"
#include "singly_linked_list.hpp"

class FreeListAllocator : public Allocator {
  public:
    enum PlacementPolicy { FindFirst, FindBest };

    FreeListAllocator(
        const uint64 total_size, const PlacementPolicy placement_policy
    );

    virtual void* allocate(const uint64 size, const uint64 alignment = 0)
        override;
    virtual void free(void* ptr) override;
    virtual void reset() override;

  private:
    struct FreeHeader {
        uint64 block_size;
    };
    struct AllocationHeader {
        uint64 block_size;
        char   padding;
    };

    typedef SinglyLinkedList<FreeHeader>::Node Node;

    PlacementPolicy              _placement_policy;
    SinglyLinkedList<FreeHeader> _free_list;

    FreeListAllocator(FreeListAllocator& free_list_allocator);

    void find(
        const uint64 size,
        const uint64 alignment,
        uint64&      padding,
        Node*&       previous_node,
        Node*&       found_node
    );
    void find_best(
        const uint64 size,
        const uint64 alignment,
        uint64&      padding,
        Node*&       previous_node,
        Node*&       found_node
    );
    void find_first(
        const uint64 size,
        const uint64 alignment,
        uint64&      padding,
        Node*&       previous_node,
        Node*&       found_node
    );

    void coalescence(Node* prev_block, Node* free_block);
};