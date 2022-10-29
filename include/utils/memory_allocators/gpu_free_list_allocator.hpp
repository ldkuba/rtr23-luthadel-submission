#pragma once

#include "forward_list.hpp"
#include "map.hpp"

class GPUFreeListAllocator : public Allocator {
  public:
    enum PlacementPolicy { FindFirst, FindBest };

    GPUFreeListAllocator(
        const uint64          total_size,
        const uint64          begin_offset,
        const PlacementPolicy placement_policy
    );

    virtual void  init() override;
    virtual void* allocate(const uint64 size, const uint64 alignment = 0)
        override;
    virtual void free(void* ptr) override;
    virtual void reset() override;

    bool allocated(const void* ptr, const uint64 size);

  private:
    struct FreeHeader {
        uint64 block_size;
        uint64 offset;
    };
    struct AllocationHeader {
        uint64 block_size;
        uint8  padding;
    };

    typedef ForwardList<FreeHeader>::iterator Node;

    PlacementPolicy               _placement_policy;
    ForwardList<FreeHeader>       _free_list {};
    Map<uint64, AllocationHeader> _allocated {};

    GPUFreeListAllocator(GPUFreeListAllocator& free_list_allocator);

    void find(
        const uint64 size,
        const uint64 alignment,
        uint64&      padding,
        Node&        previous_node,
        Node&        found_node
    );
    void find_best(
        const uint64 size,
        const uint64 alignment,
        uint64&      padding,
        Node&        previous_node,
        Node&        found_node
    );
    void find_first(
        const uint64 size,
        const uint64 alignment,
        uint64&      padding,
        Node&        previous_node,
        Node&        found_node
    );

    void coalescence(Node prev_block, Node free_block);
};
