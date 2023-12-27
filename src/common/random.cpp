#include "random.hpp"

namespace ENGINE_NAMESPACE {

std::random_device Random::_random_device {};
std::mt19937       Random::_generator { _random_device() };

// ///////////////////// //
// RANDOM PUBLIC METHODS //
// ///////////////////// //

typedef ENGINE_NAMESPACE::uint8   u8;
typedef ENGINE_NAMESPACE::uint16  u16;
typedef ENGINE_NAMESPACE::uint32  u32;
typedef ENGINE_NAMESPACE::uint64  u64;
typedef ENGINE_NAMESPACE::uint128 u128;

typedef ENGINE_NAMESPACE::int8   i8;
typedef ENGINE_NAMESPACE::int16  i16;
typedef ENGINE_NAMESPACE::int32  i32;
typedef ENGINE_NAMESPACE::int64  i64;
typedef ENGINE_NAMESPACE::int128 i128;

typedef ENGINE_NAMESPACE::float32  f32;
typedef ENGINE_NAMESPACE::float64  f64;
typedef ENGINE_NAMESPACE::float128 f128;

void Random::set_seed(const u32 seed) { _generator.seed(seed); }

u8   Random::uint8(u8 min, u8 max) { return random_integer(min, max); }
u16  Random::uint16(u16 min, u16 max) { return random_integer(min, max); }
u32  Random::uint32(u32 min, u32 max) { return random_integer(min, max); }
u64  Random::uint64(u64 min, u64 max) { return random_integer(min, max); }
u128 Random::uint128(u128 min, u128 max) { return random_integer(min, max); }

i8   Random::int8(i8 min, i8 max) { return random_integer(min, max); }
i16  Random::int16(i16 min, i16 max) { return random_integer(min, max); }
i32  Random::int32(i32 min, i32 max) { return random_integer(min, max); }
i64  Random::int64(i64 min, i64 max) { return random_integer(min, max); }
i128 Random::int128(i128 min, i128 max) { return random_integer(min, max); }

f32 Random::float32(f32 min, f32 max) { return random_real(min, max); }
f64 Random::float64(f64 min, f64 max) { return random_real(min, max); }
// f128 Random::float128(f128 min, f128 max) { return random_real(min, max); }

f32 Random::float32_01() { return random_real<f32>(0.0f, 1.0f); }
f64 Random::float64_01() { return random_real<f64>(0.0, 1.0); }
// f128 Random::float128_01() { return random_real<f128>(0.0, 1.0); }

} // namespace ENGINE_NAMESPACE