#pragma once

#include <buddy_system/arena.hpp>

namespace buddy_system {

// The allocator will be used as a handle to the buddy memory arena.
// Here, only the necessary features are implemented.
// In nearly all cases this allocator will be used by calling
// std::allocator_traits with a respective template argument. Therefore all
// optional features should be accessible through the default implementation of
// std::allocator_traits.
template <typename T>
struct allocator {
  using value_type = T;

  allocator(arena& a) noexcept : handle{a} {}
  template <typename U>
  allocator(const allocator<U>& other) noexcept : handle{other.handle} {}

  T* allocate(size_t n) {
    return reinterpret_cast<T*>(handle.allocate(n * sizeof(T)));
  }
  void deallocate(T* ptr, size_t n) noexcept {
    handle.deallocate(reinterpret_cast<void*>(ptr));
  }

  arena& handle;
};

}  // namespace buddy_system