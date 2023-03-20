#pragma once
#include "../platform.hpp"

static inline
UInt round_up_to_power_of_two(UInt size) {
  // Sanity check to prevent overflow.
  ASSERT(size < SIZE_MAX / 2);

  UInt result = 1;
  while (result < size) {
    result *= 2;
  }

  return result;
}

static inline
UInt align_down(UInt value, UInt alignment) {
  return value - value % alignment;
}

static inline
UInt align_up(UInt value, UInt alignment) {
  // Sanity check to prevent overflow.
  ASSERT(value + alignment > value);

  return value + alignment - 1 - (value - 1) % alignment;
}

template<typename T_out, typename T_in>
static inline
T_out safe_int_cast(T_in value) {
  auto forward = T_out(value);
  auto back = T_in(forward);

  // If the cast is lossy, then the value will be different after the round trip.
  ASSERT(back == value);

  return forward;
}

template<typename T, typename B, typename P>
static inline
T offset_between(B *base, P *pointer) {
  auto result = (Byte *) pointer - (Byte *) base;
  return safe_int_cast<T>(result);
}

template<typename T>
static inline
T maxi(T a, T b) {
  return (a > b ? a : b);
}

template<typename T>
static inline
T mini(T a, T b) {
  return (a < b ? a : b);
}
