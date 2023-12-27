#pragma once

#include "defines.hpp"

#include <random>

namespace ENGINE_NAMESPACE {

class Random {
  public:
    static void set_seed(const ENGINE_NAMESPACE::uint32 seed);

    static ENGINE_NAMESPACE::uint8  uint8( //
        ENGINE_NAMESPACE::uint8 min = 0,
        ENGINE_NAMESPACE::uint8 max = uint8_max
    );
    static ENGINE_NAMESPACE::uint16 uint16(
        ENGINE_NAMESPACE::uint16 min = 0,
        ENGINE_NAMESPACE::uint16 max = uint16_max
    );
    static ENGINE_NAMESPACE::uint32 uint32(
        ENGINE_NAMESPACE::uint32 min = 0,
        ENGINE_NAMESPACE::uint32 max = uint32_max
    );
    static ENGINE_NAMESPACE::uint64 uint64(
        ENGINE_NAMESPACE::uint64 min = 0,
        ENGINE_NAMESPACE::uint64 max = uint64_max
    );
    static ENGINE_NAMESPACE::uint128 uint128(
        ENGINE_NAMESPACE::uint128 min = 0,
        ENGINE_NAMESPACE::uint128 max = uint128_max
    );

    static ENGINE_NAMESPACE::int8 int8(
        ENGINE_NAMESPACE::int8 min = int8_min,
        ENGINE_NAMESPACE::int8 max = int8_max
    );
    static ENGINE_NAMESPACE::int16 int16(
        ENGINE_NAMESPACE::int16 min = int16_min,
        ENGINE_NAMESPACE::int16 max = int16_max
    );
    static ENGINE_NAMESPACE::int32 int32(
        ENGINE_NAMESPACE::int32 min = int32_min,
        ENGINE_NAMESPACE::int32 max = int32_max
    );
    static ENGINE_NAMESPACE::int64 int64(
        ENGINE_NAMESPACE::int64 min = int64_min,
        ENGINE_NAMESPACE::int64 max = int64_max
    );
    static ENGINE_NAMESPACE::int128 int128(
        ENGINE_NAMESPACE::int128 min = int128_min,
        ENGINE_NAMESPACE::int128 max = int128_max
    );

    static ENGINE_NAMESPACE::float32 float32(
        ENGINE_NAMESPACE::float32 min = float32_min,
        ENGINE_NAMESPACE::float32 max = float32_max
    );
    static ENGINE_NAMESPACE::float64 float64(
        ENGINE_NAMESPACE::float64 min = float64_min,
        ENGINE_NAMESPACE::float64 max = float64_max
    );
    // static ENGINE_NAMESPACE::float128 float128(
    //     ENGINE_NAMESPACE::float128 min = float128_min,
    //     ENGINE_NAMESPACE::float128 max = float128_max
    // );

    static ENGINE_NAMESPACE::float32 float32_01();
    static ENGINE_NAMESPACE::float64 float64_01();
    // static ENGINE_NAMESPACE::float128 float128_01();

  private:
    Random();
    ~Random();

    static std::random_device _random_device;
    static std::mt19937       _generator;

    template<typename T>
    static T random_integer(T min, T max) {
        std::uniform_int_distribution<T> distribution(min, max);
        return distribution(_generator);
    }

    template<typename T>
    static T random_real(T min, T max) {
        std::uniform_real_distribution<T> distribution(min, max);
        return distribution(_generator);
    }
};

} // namespace ENGINE_NAMESPACE