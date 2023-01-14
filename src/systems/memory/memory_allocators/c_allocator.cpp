#include "systems/memory/memory_allocators/c_allocator.hpp"

#include <stdlib.h>

CAllocator::CAllocator() : Allocator(0) {}
CAllocator::~CAllocator() {}

void  CAllocator::init() {}
void* CAllocator::allocate(const uint64 size, const uint64 alignment) {
    return malloc(size);
}
void CAllocator::free(void* ptr) { std::free(ptr); }
bool CAllocator::owns(void* ptr) { return ptr != nullptr; }
