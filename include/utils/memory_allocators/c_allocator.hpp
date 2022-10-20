#pragma once

#include "allocator.hpp"

class CAllocator : public Allocator {
  public:
    CAllocator();
    virtual ~CAllocator();

    virtual void  init() override;
    virtual void* allocate(const uint64 size, const uint64 alignment = 0)
        override;
    virtual void free(void* ptr) override;
    virtual bool owns(void* ptr) override;
};
