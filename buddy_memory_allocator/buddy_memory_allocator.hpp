#pragma once

#include <buddy_memory_allocator/utility.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

class buddy_memory_allocator {
  struct node {
    node* next{};
  };

 public:
  explicit buddy_memory_allocator(size_t);
  ~buddy_memory_allocator();
  buddy_memory_allocator(buddy_memory_allocator&) = delete;
  buddy_memory_allocator& operator=(buddy_memory_allocator&) = delete;
  buddy_memory_allocator(buddy_memory_allocator&&) = delete;
  buddy_memory_allocator& operator=(buddy_memory_allocator&&) = delete;

  void* malloc(size_t size) noexcept;
  void free(void* address) noexcept;

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

  friend std::ostream& operator<<(std::ostream&, const buddy_memory_allocator&);

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

buddy_memory_allocator::buddy_memory_allocator(size_t s) {
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

buddy_memory_allocator::~buddy_memory_allocator() { delete[] memory; }

inline void* buddy_memory_allocator::malloc(size_t size) noexcept {
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

inline void buddy_memory_allocator::free(void* address) noexcept {
  // Do nothing with an empty address.
  if (!address) return;

  // Transform pointer to whole page with header again.
  auto page = reinterpret_cast<node*>(address) - 1;
  // Get the index from the header.
  auto index = reinterpret_cast<intptr_t>(page->next);

  // Check if page was returned by buddy allocation.
  if (index < 0 || index > free_pages.size() ||
      ((page - base) * sizeof(node*) &
       ((size_t{1} << (index + min_page_size_exp)) - 1)))
    return;
  // We assume that we are the only ones that can write into unreserved memory.
  // So we only have to check if a given page is already a free page.
  for (auto it = free_pages[index]; it; it = it->next)
    if (it == page) return;
  // At this point, we know the given address was allocated by the buddy system.

  auto new_buddy = page;

  for (;; ++index) {
    // Construct mask and test numbers to test if two pages are buddies.
    const auto is_buddy_mask = ~(size_t{1} << (index + min_page_size_exp));
    const auto buddy_test = ((new_buddy - base) * sizeof(node)) & is_buddy_mask;

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
      new_buddy->next = free_pages[index];
      free_pages[index] = new_buddy;
      return;
    }
    // Otherwise, we have found the buddy page and therefore have to pop and
    // merge it with our current page.
    prev->next = buddy->next;
    new_buddy = base + buddy_test / sizeof(node);
    // We have to repeat these instructions to check if we have to do a merge
    // for the next higher page size.
  }
}

inline size_t buddy_memory_allocator::available_memory_size() const noexcept {
  size_t result{};
  for (size_t i = 0; i < free_pages.size(); ++i) {
    for (auto it = free_pages[i]; it; it = it->next)
      result += (size_t{1} << (i + min_page_size_exp));
  }
  return result;
}

inline size_t buddy_memory_allocator::max_available_page_size() const noexcept {
  for (auto i = free_pages.size(); i > 0; --i)
    if (free_pages[i - 1]) return size_t{1} << (i - 1 + min_page_size_exp);
  return 0;
}

inline std::ostream& operator<<(std::ostream& os,
                                const buddy_memory_allocator& bs) {
  using namespace std;

  os << setfill('-') << setw(80) << '\n'
     << setfill(' ')  //
     << "allocator struct size  = " << setw(20) << sizeof(bs) << " B" << '\n'
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
  return os << setfill('-') << setw(80) << '\n' << setfill(' ');
}
