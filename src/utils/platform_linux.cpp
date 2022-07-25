#include "platform.hpp"
#if PLATFORM == LINUX

Platform::Platform() {
    std::cout << "PLATFORM IS LINUX" << std::endl;
}

Platform::~Platform() {}

#endif