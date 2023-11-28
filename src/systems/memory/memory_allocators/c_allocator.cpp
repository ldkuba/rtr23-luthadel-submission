#include "systems/memory/memory_allocators/c_allocator.hpp"

#include <stdlib.h>

namespace ENGINE_NAMESPACE {

CAllocator::CAllocator() : Allocator(0) { _start_ptr = (void*) uint64_max; }
CAllocator::~CAllocator() {}

void  CAllocator::init() {}
void* CAllocator::allocate(const uint64 size, const uint64 alignment) {
    return ::operator new(size);
}
void CAllocator::free(void* ptr) { ::operator delete(ptr); }
bool CAllocator::owns(void* ptr) { return ptr != nullptr; }

} // namespace ENGINE_NAMESPACE