#pragma once

#include "defines.hpp"

namespace ENGINE_NAMESPACE {

#define ALLOCATOR_LOG "Allocator :: "

/**
 * @brief Interface for a generic memory allocator. Provides elementary methods
 * required of any allocator.
 *
 */
class Allocator {
  public:
    /// @brief Returns starting address of owned memory if available (def = 0)
    uint64 start() { return (uint64) _start_ptr; }
    /// @brief Total size allocatable by this allocator
    uint64 total_size() { return _total_size; };
    /// @brief Memory currently used
    uint64 used() { return _used; };
    /// @brief Peek memory usage of this allocator
    uint64 peak() { return _peak; };

    Allocator(const uint64 total_size)
        : _total_size { total_size }, _used { 0 }, _peak { 0 } {}
    virtual ~Allocator();

    /**
     * @brief Initialize allocator. Must be called before any (de)allocation
     * operations.
     *
     */
    virtual void  init();
    /**
     * @brief Allocate memory segment
     *
     * @param size Size requirement in bytes
     * @param alignment Required memory alignment (by default disabled).
     * @return void* to the beginning of the allocated segment
     */
    virtual void* allocate(const uint64 size, const uint64 alignment = 0);
    /**
     * @brief Free allocated memory
     *
     * @param ptr Pointer to an allocated segment (Behavior for invalid input is
     * determined by the specific allocator implementation)
     */
    virtual void  free(void* ptr);
    /**
     * @brief Resets all allocations (Relevant only for some allocators)
     *
     */
    virtual void  reset();
    /**
     * @brief Checks if a memory location was allocated by this allocator
     *
     * @param ptr Pointer to a location in memory
     * @returns true If relevant memory was allocated by this allocator
     * @returns false Otherwise
     */
    virtual bool  owns(void* ptr);

  protected:
    void*  _start_ptr = nullptr;
    uint64 _total_size;
    uint64 _used;
    uint64 _peak;

    static const uint64 calculate_padding(
        const uint64 base_address, const uint64 alignment
    ) {
        return get_aligned(base_address, alignment) - base_address;
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

} // namespace ENGINE_NAMESPACE