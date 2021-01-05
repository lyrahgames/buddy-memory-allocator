#pragma once
#include <lyrahgames/buddy_system/arena.hpp>

inline void* operator new(size_t size, lyrahgames::buddy_system::arena& a) {
  return a.allocate(size);
}

inline void* operator new[](size_t size, lyrahgames::buddy_system::arena& a) {
  return a.allocate(size);
}

inline void operator delete(void* ptr,
                            lyrahgames::buddy_system::arena& a) noexcept {
  a.free(ptr);
}

inline void operator delete[](void* ptr,
                              lyrahgames::buddy_system::arena& a) noexcept {
  a.free(ptr);
}
