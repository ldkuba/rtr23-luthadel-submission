#pragma once

#include <limits>

// Global settings
#define ENGINE_NAME "Vulkan Engine"
#define APP_NAME "Vulkan Engine"
#define ENGINE_NAMESPACE engine_namespace

namespace ENGINE_NAMESPACE {

// -----------------------------------------------------------------------------
// TYPES
// -----------------------------------------------------------------------------
// byte
typedef char          byte;
typedef unsigned char ubyte;

// Unsigned char
typedef unsigned char uchar;

// Unsigned integer
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;
typedef __uint128_t        uint128;

// Signed integer
typedef signed char      int8;
typedef signed short     int16;
typedef signed int       int32;
typedef signed long long int64;
typedef __int128_t       int128;

// Floats
typedef float      float32;
typedef double     float64;
typedef __float128 float128;

// Check if sizes are correct
static_assert(sizeof(byte) == 1, "Expected a 1 byte long byte.");
static_assert(sizeof(ubyte) == 1, "Expected a 1 byte long ubyte.");

static_assert(sizeof(uchar) == 1, "Expected a 1 byte long uchar.");

static_assert(sizeof(uint8) == 1, "Expected a 1 byte long uint8.");
static_assert(sizeof(uint16) == 2, "Expected a 2 byte long uint16.");
static_assert(sizeof(uint32) == 4, "Expected a 4 byte long uint32.");
static_assert(sizeof(uint64) == 8, "Expected a 8 byte long uint64.");
static_assert(sizeof(uint128) == 16, "Expected a 16 byte long uint128.");

static_assert(sizeof(int8) == 1, "Expected a 1 byte long int8.");
static_assert(sizeof(int16) == 2, "Expected a 2 byte long int16.");
static_assert(sizeof(int32) == 4, "Expected a 4 byte long int32.");
static_assert(sizeof(int64) == 8, "Expected a 8 byte long int64.");
static_assert(sizeof(int128) == 16, "Expected a 16 byte long int128.");

static_assert(sizeof(float32) == 4, "Expected a 4 byte long float32.");
static_assert(sizeof(float64) == 8, "Expected a 8 byte long float64.");
static_assert(sizeof(float128) == 16, "Expected a 16 byte long float128.");

// String constant expresion attribute
#define StringEnum constexpr static const char* const
#define STRING_ENUM(x) StringEnum x = #x

// -----------------------------------------------------------------------------
// Numeric limits
// -----------------------------------------------------------------------------

// Max unsigned integers
inline const constexpr uint8   uint8_max   = 255;
inline const constexpr uint16  uint16_max  = 65535;
inline const constexpr uint32  uint32_max  = 4294967295U;
inline const constexpr uint64  uint64_max  = (uint64) -1;
inline const constexpr uint128 uint128_max = (uint128) -1;

// Max / Min integers
inline const constexpr int8   int8_max   = 127;
inline const constexpr int16  int16_max  = 32767;
inline const constexpr int32  int32_max  = 2147483647;
inline const constexpr int64  int64_max  = uint64_max / 2;
inline const constexpr int128 int128_max = uint128_max / 2;

inline const constexpr int8   int8_min   = -int8_max - 1;
inline const constexpr int16  int16_min  = -int16_max - 1;
inline const constexpr int32  int32_min  = -int32_max - 1;
inline const constexpr int64  int64_min  = -int64_max - 1;
inline const constexpr int128 int128_min = -uint128_max - 1;

// Smallest positive floats
inline const constexpr float32 Epsilon32 =
    std::numeric_limits<float32>::epsilon();
inline const constexpr float64 Epsilon64 =
    std::numeric_limits<float64>::epsilon();
inline const constexpr float64 Epsilon128 =
    std::numeric_limits<float128>::epsilon();
inline const constexpr float32 Infinity32 =
    std::numeric_limits<float32>::infinity();
inline const constexpr float64 Infinity64 =
    std::numeric_limits<float64>::infinity();
inline const constexpr float128 Infinity128 =
    std::numeric_limits<float128>::infinity();

// -----------------------------------------------------------------------------
// PLATFORMS
// -----------------------------------------------------------------------------
// List of supported platforms
#define LINUX 1
#define WINDOWS32 2
#define WINDOWS64 3

// detect platform
#ifdef _WIN32 // Includes both 32 bit and 64 bit
#    ifdef _WIN64
#        define PLATFORM WINDOWS32
#    else
#        define PLATFORM WINDOWS64
#    endif
#else
#    if __linux__
#        define PLATFORM LINUX
#    elif __unix__
#        define PLATFORM UNIX
#    else
#        error "Cant compile on this platform.";
#    endif
#endif

// Some widly used methods
inline uint64 get_aligned(uint64 operand, uint64 granularity) {
    return ((operand + (granularity - 1)) & ~(granularity - 1));
}

} // namespace ENGINE_NAMESPACE