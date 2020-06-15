#pragma once
#include <buddy_system/arena.hpp>

inline void* operator new(size_t size, buddy_system::arena& a) {
  return a.allocate(size);
}

inline void* operator new[](size_t size, buddy_system::arena& a) {
  return a.allocate(size);
}

inline void operator delete(void* ptr, buddy_system::arena& a) noexcept {
  a.free(ptr);
}

inline void operator delete[](void* ptr, buddy_system::arena& a) noexcept {
  a.free(ptr);
}
