#include "platform/platform.hpp"
#if PLATFORM == WINDOWS32

Platform::Platform() {
    std::cout << "PLATFORM IS WINDOWS32" << std::endl;
}

Platform::~Platform() {}

#endif