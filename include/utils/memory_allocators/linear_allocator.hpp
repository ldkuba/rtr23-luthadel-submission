#pragma once

#include "allocator.hpp"

class LinearAllocator : public Allocator {
  public:
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