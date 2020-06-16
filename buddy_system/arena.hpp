#pragma once

#include <buddy_system/utility.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace buddy_system {

// The arena is the actual buddy system that is managing memory manually
// allocated on the heap. It provides bare-bones allocation and deallocation
// functions and will accessed by an allocator which acts as a handle to the
// buddy system algorithms.
class arena {
  struct node {
    node* next{};
  };

 public:
  explicit arena(size_t);
  ~arena();
  arena(arena&) = delete;
  arena& operator=(arena&) = delete;
  arena(arena&&) = delete;
  arena& operator=(arena&&) = delete;

  void* malloc(size_t size) noexcept;
  void free(void* address) noexcept;

  void* allocate(size_t size) {
    const auto result = malloc(size);
    if (!result) throw std::bad_alloc{};
    return result;
  }
  void deallocate(void* address) noexcept { free(address); }

  size_t min_page_size() const noexcept {
    return size_t{1} << min_page_size_exp;
  }
  size_t max_page_size() const noexcept {
    return size_t{1} << max_page_size_exp;
  }
  size_t page_size(void* ptr) const noexcept {
    return size_t{1} << (reinterpret_cast<size_t>(
                             (reinterpret_cast<node*>(ptr) - 1)->next) +
                         min_page_size_exp);
  }
  size_t managed_memory_size() const noexcept {
    return size_t{1} << max_page_size_exp;
  }
  size_t reserved_memory_size() const noexcept { return memory_size; }
  size_t available_memory_size() const noexcept;
  size_t max_available_page_size() const noexcept;
  auto index_of_node_ptr(node* ptr) const noexcept {
    return (ptr - base) * sizeof(node*);
  }
  auto index_of_node_ptr(void* ptr) const noexcept {
    return index_of_node_ptr(reinterpret_cast<node*>(ptr));
  }
  auto node_ptr_of_index(intptr_t index) const noexcept {
    return base + index / sizeof(node*);
  }
  auto next_size_exp(size_t size) const noexcept {
    return std::max(min_page_size_exp, size_t(log2(size - 1) + 1));
  }
  bool is_valid(void* ptr) const noexcept;

  friend std::ostream& operator<<(std::ostream&, const arena&);

 private:
  static constexpr size_t page_header_size = alignof(node);
  static constexpr size_t page_alignment{64};
  size_t max_page_size_exp{};
  size_t min_page_size_exp{6};
  size_t memory_size{};
  node* base{};
  std::vector<node*> free_pages{};
  std::byte* memory{};
};

arena::arena(size_t s) {
  // We do not support management of memory with size zero.
  if (!s) throw std::bad_alloc{};

  // Compute next power of two for memory management size.
  // Managed memory needs to have its size to be equal to a power of two.
  // This size will then be the maximal page size.
  max_page_size_exp = next_size_exp(s);
  const auto size = size_t{1} << max_page_size_exp;

  // Allocate aligned system memory on the heap to be managed.
  memory_size = size + page_alignment;
  memory = new (std::align_val_t{page_alignment}) std::byte[memory_size];

  // Base pointer is only 8-byte aligned but the actual returned memory pointers
  // will be 64-byte aligned.
  base = reinterpret_cast<node*>(memory + (page_alignment - page_header_size));

  // Allocate free buddy list and initialize it.
  free_pages =
      decltype(free_pages)(max_page_size_exp - min_page_size_exp + 1, nullptr);
  free_pages.back() = base;
  free_pages.back()->next = nullptr;
}

arena::~arena() { delete[] memory; }

inline void* arena::malloc(size_t size) noexcept {
  // We do not support allocating memory with size zero.
  if (!size) return nullptr;
  // Compute the actual size of the page by calculating the next power of two
  // bucket.
  const auto page_size_exp = next_size_exp(size + page_header_size);
  // Check for too large page size.
  if (page_size_exp > max_page_size_exp) return nullptr;
  // Search for a possible split index starting from the given page size.
  const int index = page_size_exp - min_page_size_exp;
  for (int split = index; split < free_pages.size(); ++split) {
    if (free_pages[split]) {
      // When found, pop the split buddy.
      const auto result = free_pages[split];
      free_pages[split] = result->next;
      // Iterate over all resulting free splitted buddies.
      for (int i = split - 1; i >= index; --i) {
        // Push those new free splitted buddies by first computing their
        // address. We need to take care of pointer arithmetic because we are
        // counting bytes.
        free_pages[i] =
            result + ((size_t{1} << (min_page_size_exp + i)) / sizeof(node*));
        // We know that the list was previously empty.
        free_pages[i]->next = nullptr;
      }
      // Write index into the header of the page.
      result->next = reinterpret_cast<node*>(index);
      // Return the allocated splitted buddy without the header to the user.
      // This address again has to be 64-byte aligned because we made sure to
      // allocate the underlying memory with a 64-byte alignment and moved the
      // base pointer 8 byte to the left to give space for the page header.
      return reinterpret_cast<void*>(result + 1);
    }
  }
  // If there was no possible split index, we do not have enough memory.
  return nullptr;
}

inline bool arena::is_valid(void* ptr) const noexcept {
  if (!ptr) return false;
  // Cast difference to unsigned integer to make bounds testing easier.
  const auto page = reinterpret_cast<node*>(ptr) - 1;
  const auto memory_index = static_cast<size_t>(page - base);
  // The pointer should lie in the space of our allocated memory.
  if (memory_index >= managed_memory_size()) return false;
  // Now we can access memory without segmentation fault and are able to ask for
  // the pages size. Again, we use an unsigned integer for easier bounds
  // testing.
  const auto index = reinterpret_cast<size_t>(page->next);
  if (index >= free_pages.size()) return false;
  // Address must provide the alignment of its page size.
  if (memory_index & ((size_t{1} << (index + min_page_size_exp)) - size_t{1}))
    return false;
  // Check if the existing page is already a free page.
  for (auto it = free_pages[index]; it; it = it->next)
    if (it == page) return false;
  return true;
}

inline void arena::free(void* address) noexcept {
  // First, we have to check the validity of the given address.
  // We assume that we are the only ones that can write into unreserved memory.

  // Do nothing with an empty address.
  if (!address) return;
  // Cast difference to unsigned integer to make bounds testing easier.
  auto page = reinterpret_cast<node*>(address) - 1;
  const auto memory_index = index_of_node_ptr(page);
  // The pointer should lie in the space of our allocated memory.
  if (memory_index >= managed_memory_size()) return;
  // Now we can access memory without segmentation fault and are able to ask for
  // the pages size. Again, we use an unsigned integer for easier bounds
  // testing.
  auto index = reinterpret_cast<size_t>(page->next);
  if (index >= free_pages.size()) return;
  // Address must provide the alignment of its page size.
  if (memory_index & ((size_t{1} << (index + min_page_size_exp)) - size_t{1}))
    return;
  // Check if the existing page is already a free page.
  for (auto it = free_pages[index]; it; it = it->next)
    if (it == page) return;

  // At this point, we know the given address was allocated by the buddy system.

  for (;; ++index) {
    // Construct mask and test numbers to test if two pages are buddies.
    const auto is_buddy_mask = ~(size_t{1} << (index + min_page_size_exp));
    const auto buddy_test = index_of_node_ptr(page) & is_buddy_mask;

    // Go trough the list of pages with this size and look for the possible
    // merge buddy.
    node* buddy = nullptr;
    auto prev = reinterpret_cast<node*>(&free_pages[index]);
    for (auto it = free_pages[index]; it; prev = it, it = it->next) {
      // Test if the current page in the list is the buddy.
      if ((((it - base) * sizeof(node)) & is_buddy_mask) == buddy_test) {
        buddy = it;
        break;
      }
    }
    // If there is no buddy for merging, push the given page to the list.
    // At some point, this will always be true.
    if (buddy == nullptr) {
      page->next = free_pages[index];
      free_pages[index] = page;
      return;
    }
    // Otherwise, we have found the buddy page and therefore have to pop and
    // merge it with our current page.
    prev->next = buddy->next;
    page = base + buddy_test / sizeof(node);
    // We have to repeat these instructions to check if we have to do a merge
    // for the next higher page size.
  }
}

inline size_t arena::available_memory_size() const noexcept {
  size_t result{};
  for (size_t i = 0; i < free_pages.size(); ++i) {
    for (auto it = free_pages[i]; it; it = it->next)
      result += (size_t{1} << (i + min_page_size_exp));
  }
  return result;
}

inline size_t arena::max_available_page_size() const noexcept {
  for (auto i = free_pages.size(); i > 0; --i)
    if (free_pages[i - 1]) return size_t{1} << (i - 1 + min_page_size_exp);
  return 0;
}

inline std::ostream& operator<<(std::ostream& os, const arena& bs) {
  using namespace std;

  os << setfill('-') << setw(80) << '\n'
     << setfill(' ')  //
     << "arena struct size  = " << setw(20) << sizeof(bs) << " B" << '\n'
     << '\n'
     << "offset of buddy memory = " << setw(20) << bs.memory << '\n'
     << "size of buddy memory   = " << setw(20) << bs.memory_size << " B"
     << '\n'
     << "alignment buddy memory = " << setw(20) << alignment_of_ptr(bs.memory)
     << " B"
     << '\n'
     // << '\n'
     << "base pointer offset    = " << setw(20) << bs.base << '\n'
     << "managed memory size    = " << setw(20) << bs.managed_memory_size()
     << " B" << '\n'
     << "alignment base pointer = " << setw(20) << alignment_of_ptr(bs.base)
     << " B" << '\n'
     << '\n'
     // << "unused memory offset   = " << setw(20)
     // << decltype(bs.memory)(bs.base) - bs.memory << " B" << '\n'
     // << "unused memory end      = " << setw(20)
     // << (bs.memory + bs.memory_size) -
     //        (decltype(bs.memory)(bs.base) + bs.managed_memory_size())
     // << " B" << '\n'
     // << "unused memory          = " << setw(20)
     // << bs.memory_size - bs.managed_memory_size() << " B" << '\n'
     // << '\n'
     << "page header size       = " << setw(20) << bs.page_header_size << " B"
     << '\n'
     << "page alignment         = " << setw(20) << bs.page_alignment << " B"
     << '\n'
     << "maximal page size      = " << setw(20) << bs.max_page_size() << " B"
     << '\n'
     << "maximal page size exp  = " << setw(20) << bs.max_page_size_exp << '\n'
     << "minimal page size      = " << setw(20) << bs.min_page_size() << " B"
     << '\n'
     << "minimal page size exp  = " << setw(20) << bs.min_page_size_exp << '\n'
     << '\n'
     << "free pages lists size  = " << setw(20) << bs.free_pages.size() << '\n'
     << "free pages lists memory= " << setw(20)
     << bs.free_pages.size() * sizeof(bs.free_pages[0]) << " B" << '\n'
     << "available memory size  = " << setw(20) << bs.available_memory_size()
     << " B" << '\n'
     << "max available page size= " << setw(20) << bs.max_available_page_size()
     << " B" << '\n'
     << '\n'
     << "free pages lists content:" << '\n';

  for (int i = bs.max_page_size_exp; i >= bs.min_page_size_exp; --i) {
    os << setw(4) << "2^" << setw(2) << i << " B = " << setw(10) << (1ull << i)
       << " B :";
    for (auto it = bs.free_pages[i - bs.min_page_size_exp]; it; it = it->next)
      os << setw(5) << "-->" << setw(12)
         << (it - bs.base) * sizeof(bs.free_pages[0]) << " (" << it << ")";
    os << '\n';
  }
  os << '\n';

  // Memory Allocation Scheme
  const size_t scheme_size_exp = std::min(size_t{6}, bs.max_page_size_exp);
  auto scheme_size = (size_t{1} << scheme_size_exp) + 1;
  char scheme[scheme_size];
  for (size_t i = 0; i < scheme_size - 1; ++i) scheme[i] = '=';

  const auto start_exp = std::max(bs.max_page_size_exp - scheme_size_exp + 1,
                                  bs.min_page_size_exp);

  for (size_t i = start_exp; i <= bs.max_page_size_exp; ++i) {
    for (auto it = bs.free_pages[i - bs.min_page_size_exp]; it; it = it->next) {
      const auto size = size_t{1} << i;
      const auto index = bs.index_of_node_ptr(it);
      const auto length = size >> (bs.max_page_size_exp - scheme_size_exp);
      auto scheme_index = index >> (bs.max_page_size_exp - scheme_size_exp);
      scheme[scheme_index] = '[';
      for (size_t j = 1; j < length - 1; ++j) scheme[scheme_index + j] = '-';
      scheme[scheme_index + length - 1] = ']';
    }
  }
  for (size_t i = bs.min_page_size_exp; i < start_exp; ++i) {
    for (auto it = bs.free_pages[i - bs.min_page_size_exp]; it; it = it->next) {
      const auto index = bs.index_of_node_ptr(it);
      auto scheme_index = index >> (bs.max_page_size_exp - scheme_size_exp);
      scheme[scheme_index] = '|';
    }
  }
  scheme[scheme_size - 1] = '\0';
  os << "Memory Layout Scheme of Free Pages: "
     << "(" << (size_t{1} << (start_exp - 1)) << " B/char)" << '\n'
     << scheme << '\n';

  return os << setfill('-') << setw(80) << '\n' << setfill(' ');
}

}  // namespace buddy_system