#pragma once

#include <cstddef>
#include <cstdint>

namespace lyrahgames::buddy_system {

inline unsigned next_power_of_2(unsigned size) {
  /* depend on the fact that size < 2^32 */
  size -= 1;
  size |= (size >> 1);
  size |= (size >> 2);
  size |= (size >> 4);
  size |= (size >> 8);
  size |= (size >> 16);
  return size + 1;
}

// int log2(int x) { return __builtin_ctz(x); }
inline uint32_t log2(const uint32_t x) {
  uint32_t y;
  asm("\tbsr %1, %0\n" : "=r"(y) : "r"(x));
  return y;
}

inline uint64_t log2(uint64_t x) noexcept {
  uint64_t y;
  asm("\tbsr %1, %0\n" : "=r"(y) : "r"(x));
  return y;
}

inline uint32_t next_pow2(uint32_t x) {
  return uint32_t{1} << (log2(x - 1) + 1);
}

inline intptr_t alignment_of_ptr(void* ptr) noexcept {
  if (!ptr) return 0;
  const auto p = reinterpret_cast<intptr_t>(ptr);
  intptr_t alignment = 1;
  for (; !(alignment & p); alignment = (alignment << 1) | 0x1)
    ;
  return (alignment + 1) >> 1;
}

}  // namespace lyrahgames::buddy_system