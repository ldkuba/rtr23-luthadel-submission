#pragma once

#include "defines.hpp"

#define ALLOCATOR_LOG "Allocator :: "

class Allocator {
  public:
    Allocator(const uint64 total_size)
        : _total_size { total_size }, _used { 0 }, _peak { 0 } {}
    virtual ~Allocator();

    virtual void  init();
    virtual void* allocate(const uint64 size, const uint64 alignment = 0);
    virtual void  free(void* ptr);
    virtual void  reset();
    virtual bool  owns(void* ptr);

  protected:
    void*  _start_ptr = nullptr;
    uint64 _total_size;
    uint64 _used;
    uint64 _peak;

    static const uint64 calculate_padding(
        const uint64 base_address, const uint64 alignment
    ) {
        if (base_address % alignment == 0) return 0;
        const uint64 multiplier      = (base_address / alignment) + 1;
        const uint64 aligned_address = multiplier * alignment;
        return aligned_address - base_address;
    }

    static const uint64 calculate_padding_with_header(
        const uint64 base_address,
        const uint64 alignment,
        const uint64 header_size
    ) {
        return header_size +
               calculate_padding(base_address + header_size, alignment);
    }
};
