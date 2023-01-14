#include "systems/memory/memory_allocators/allocator.hpp"

#include <stdlib.h> /* malloc, free */

Allocator::~Allocator() {
    free(_start_ptr);
    _start_ptr = nullptr;
}

void Allocator::init() {
    if (_start_ptr != nullptr) { free(_start_ptr); }
    _start_ptr = malloc(_total_size);
    this->reset();
}
void* Allocator::allocate(const uint64 size, const uint64 alignment) {
    return nullptr;
}
void Allocator::free(void* ptr) {}
void Allocator::reset() {}
bool Allocator::owns(void* ptr) {
    return ptr >= _start_ptr &&
           (uint64) ptr < (uint64) _start_ptr + _total_size;
}