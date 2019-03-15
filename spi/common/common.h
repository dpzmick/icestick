#pragma once

inline bool is_pow_two(uint64_t x)
{
  return 1 == __builtin_popcount(x);
}
