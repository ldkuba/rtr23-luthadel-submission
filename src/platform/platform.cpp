#include "platform/platform.hpp"

const int n = 1;

// little endian if true
const bool Platform::is_little_endian = *(char*) &n == 1;