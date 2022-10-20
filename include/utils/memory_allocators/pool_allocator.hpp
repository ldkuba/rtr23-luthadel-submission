#pragma once

#include "allocator.hpp"
#include "stack_linked_list.hpp"

class PoolAllocator : public Allocator {
  public:
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