#pragma once

// Global settings
#define ENGINE_NAME "Vulkan Engine"
#define APP_NAME "Vulkan Engine"
#define ENGINE_NAMESPACE engine_namespace

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

// Additional max values
#define UINT128_MAX ((uint128) -1)
#define INT128_MAX ((int128) (UINT128_MAX / 2))
#define INT128_MIN ((int128) - (UINT128_MAX / 2 + 1))

// String constant expresion attribute
#define STRING_CONST(x) constexpr static const char* const x = #x

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