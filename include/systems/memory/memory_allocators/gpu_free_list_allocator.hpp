#pragma once

#include "forward_list.hpp"
#include "map.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Free list allocator specialized for management of GPU memory. Used
 * mostly for management of GPU buffers from the host. Segment headers are saved
 * in host memory, while GPU local memory houses only the actual data.
 * Allocations will return only a relative address (compared to some initial in
 * buffer offset), rather then a full GPU memory specific address. Otherwise
 * this allocator is in function identical to Free list allocator.
 */
class GPUFreeListAllocator : public Allocator {
  public:
    enum PlacementPolicy { FindFirst, FindBest };

    /**
     * @brief Construct a new GPUFreeListAllocator object
     *
     * @param total_size Maximum possible total size this allocator can
     * allocate
     * @param begin_offset Initial in-buffer offset. Allocations will return
     * addresses in relation to this offset.
     * @param placement_policy Placement placement policy used when deciding
     * which of the free segments will be used for allocation
     */
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

    /**
     * @brief Check if a memory segment is allocated here by this allocator.
     *
     * @param ptr Memory location (relative to initial in-buffer offset) of
     * segment's beginning
     * @param size Segment size
     * @returns true If a segment of this size is allocated at this location
     * @returns false Otherwise
     */
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

} // namespace ENGINE_NAMESPACE