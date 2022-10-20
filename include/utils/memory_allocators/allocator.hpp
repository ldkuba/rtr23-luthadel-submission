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
        const uint64 multiplier      = (base_address / alignment) + 1;
        const uint64 aligned_address = multiplier * alignment;
        const uint64 padding         = aligned_address - base_address;
        return padding;
    }

    static const uint64 calculate_padding_with_header(
        const uint64 base_address,
        const uint64 alignment,
        const uint64 header_size
    ) {
        uint64 padding      = calculate_padding(base_address, alignment);
        uint64 needed_space = header_size;

        if (padding < needed_space) {
            // Header does not fit
            // Calculate next aligned address that header fits
            needed_space -= padding;

            // How many alignments I need to fit the header
            if (needed_space % alignment > 0)
                padding += alignment * (1 + (needed_space / alignment));
            else padding += alignment * (needed_space / alignment);
        }

        return padding;
    }
};
