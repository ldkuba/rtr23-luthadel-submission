#pragma once

#include "allocator.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Stack allocator. Reserves a chunk of memory which then operates like a
 * LIFO queue (stack). Allocations take memory from the top of the stack, while
 * deallocations deallocate all memory higher on the stack.
 *
 */
class StackAllocator : public Allocator {
  public:
    StackAllocator(const uint64 total_size);

    virtual void* allocate(const uint64 size, const uint64 alignment = 0)
        override;
    virtual void free(void* ptr) override;
    virtual void reset() override;

  protected:
    uint64 _offset;

  private:
    StackAllocator(StackAllocator& stack_allocator);

    struct AllocationHeader {
        uint8 padding;
    };
};

} // namespace ENGINE_NAMESPACE