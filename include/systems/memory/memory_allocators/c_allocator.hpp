#pragma once

#include "allocator.hpp"

namespace ENGINE_NAMESPACE {

/**
 * @brief Standard C allocator. Uses malloc() and free() for allocation and
 * deallocation respectively. Initialization of this allocator is unnecessary.
 * Method own() will return false only for a nullptr. Reset does nothing.
 *
 */
class CAllocator : public Allocator {
  public:
    /**
     * @brief Construct a new CAllocator object
     *
     */
    CAllocator();
    virtual ~CAllocator();

    virtual void  init() override;
    virtual void* allocate(const uint64 size, const uint64 alignment = 0)
        override;
    virtual void free(void* ptr) override;
    virtual bool owns(void* ptr) override;
};

} // namespace ENGINE_NAMESPACE