#pragma once

#include "allocator.hpp"

/**
 * @brief Linear allocator. Reserves a chunk of memory which can then be freely
 * allocated with allocations of any size. Allocations are made one after
 * another in linear fashion. Disallows all deallocations, except for total
 * memory reset of reserved segment.
 */
class LinearAllocator : public Allocator {
  public:
    /**
     * @brief Construct a new Linear Allocator object
     *
     * @param total_size Maximum possible total size this allocator can
     * allocate
     */
    LinearAllocator(const uint64 total_size);

    virtual void* allocate(const uint64 size, const uint64 alignment = 0)
        override;
    virtual void free(void* ptr) override;
    virtual void reset() override;

  protected:
    uint64 _offset;

  private:
    LinearAllocator(LinearAllocator& linear_allocator);
};