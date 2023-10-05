#pragma once

#include "allocator.hpp"
#include "stack_linked_list.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Pool allocator. Reserves a pice of memory which it then can only
 * populate with the same fixed sized chunks. Each (de)allocation will
 * (de)allocate one chuck.
 */
class PoolAllocator : public Allocator {
  public:
    /**
     * @brief Construct a new Pool Allocator object
     *
     * @param total_size Maximum possible total size this allocator can
     * allocate
     * @param chunk_size Size of individual chunks.
     */
    PoolAllocator(const uint64 total_size, const uint64 chunk_size);

    virtual void* allocate(const uint64 size, const uint64 alignment = 0)
        override;
    virtual void free(void* ptr) override;
    virtual void reset() override;

  private:
    struct FreeHeader {};
    using Node = StackLinkedList<FreeHeader>::Node;
    StackLinkedList<FreeHeader> _free_list;

    uint64 _chunk_size;

    PoolAllocator(PoolAllocator& pool_allocator);
};

} // namespace ENGINE_NAMESPACE