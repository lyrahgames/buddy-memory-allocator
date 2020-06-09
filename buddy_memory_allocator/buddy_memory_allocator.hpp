#pragma once

#include <buddy_memory_allocator/utility.hpp>
#include <cstddef>
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
  static constexpr std::size_t header_size = alignof(node);
  static constexpr std::size_t page_alignment{64};

  static constexpr int node_size_exp = 3;

 public:
  explicit buddy_memory_allocator(std::size_t);
  ~buddy_memory_allocator();

  void* allocate(int expo);
  void deallocate(void* address, int expo);

  void* malloc(std::size_t size) noexcept;
  void free(void* address) noexcept;

  friend std::ostream& operator<<(std::ostream&, const buddy_memory_allocator&);

 private:
  std::size_t size{};
  std::size_t max_page_size{};
  std::size_t max_page_size_exp{};
  std::size_t min_page_size{};
  std::size_t min_page_size_exp{};
  std::size_t memory_size{};
  node* base{};
  std::vector<node*> free_buddy{};
  std::byte* memory{};
};

buddy_memory_allocator::buddy_memory_allocator(std::size_t s) {
  // We do not support management of memory with size zero.
  if (!s) throw std::bad_alloc{};

  // Set minimal page size with respective exponent.
  min_page_size_exp = 6;
  min_page_size = 1ull << min_page_size_exp;

  // Compute next power of two for memory management size.
  // Managed memory needs to have its size to be equal to a power of two.
  const auto size_exp =
      std::max(min_page_size_exp, std::size_t(log2(s - 1) + 1));
  size = 1ull << size_exp;

  // Set maximal page size with respective exponent.
  max_page_size_exp = size_exp;
  max_page_size = size;

  // Allocate aligned system memory on the heap to be managed.
  memory_size = size + page_alignment;
  memory = new (std::align_val_t{page_alignment}) std::byte[memory_size];

  // Base pointer is only 8-byte aligned but the actual returned memory pointers
  // will be 64-byte aligned.
  base = reinterpret_cast<node*>(memory + (page_alignment - header_size));

  // Allocate free buddy list and initialize it.
  free_buddy =
      decltype(free_buddy)(max_page_size_exp - min_page_size_exp + 1, nullptr);
  free_buddy.back() = base;
  free_buddy.back()->next = nullptr;
}

buddy_memory_allocator::~buddy_memory_allocator() { delete[] memory; }

inline void* buddy_memory_allocator::allocate(int expo) {
  if (max_page_size_exp < expo || expo < min_page_size_exp) return nullptr;
  const int index = expo - min_page_size_exp;
  int split_index = 0;
  for (int i = index; i <= max_page_size_exp - min_page_size_exp; ++i) {
    if (free_buddy[i] != nullptr) {
      split_index = i;
      break;
    }
  }
  if (split_index == 0) return nullptr;

  // pop split buddy
  const auto result = free_buddy[split_index];
  free_buddy[split_index] = result->next;

  for (int i = split_index - 1; i >= index; --i) {
    // push splitted buddy
    free_buddy[i] = result + (1 << (min_page_size_exp - node_size_exp + i));
    free_buddy[i]->next = nullptr;
  }

  return reinterpret_cast<void*>(result);
}

inline void buddy_memory_allocator::deallocate(void* address, int expo) {
  if (!address) return;
  int index = expo - min_page_size_exp;
  auto new_buddy = reinterpret_cast<node*>(address);

  while (true) {
    const auto is_buddy_mask =
        ~(static_cast<intptr_t>(1) << (index + min_page_size_exp));
    const auto buddy_test = ((new_buddy - base) * sizeof(node)) & is_buddy_mask;
    node* merge_buddy = nullptr;
    auto prev = reinterpret_cast<node*>(&free_buddy[index]);
    for (auto it = free_buddy[index]; it; prev = it, it = it->next) {
      // test if it is a buddy
      if ((((it - base) * sizeof(node)) & is_buddy_mask) == buddy_test) {
        merge_buddy = it;
        break;
      }
    }
    // if there is no buddy for merging, push buddy to list
    if (merge_buddy == nullptr) {
      // cout << "no merge buddy found\n" << flush;
      new_buddy->next = free_buddy[index];
      free_buddy[index] = new_buddy;
      return;
    }
    // we have merge buddy, so pop it and merge
    prev->next = merge_buddy->next;
    new_buddy = base + buddy_test / sizeof(node);
    ++index;
  }
}

inline void* buddy_memory_allocator::malloc(std::size_t size) noexcept {
  // We do not support allocating memory with size zero.
  if (!size) return nullptr;

  // Compute the actual size of the page by calculating the next power of two
  // bucket.
  const auto page_size_exp = std::max(
      min_page_size_exp, std::size_t(log2(size + header_size - 1) + 1));
  const auto page_size = 1 << page_size_exp;

  // Check for too large page size.
  if (page_size_exp > max_page_size_exp) return nullptr;

  // Search for a possible split index starting from the given page size.
  const int index = page_size_exp - min_page_size_exp;
  for (int split = index; split < free_buddy.size(); ++split) {
    if (free_buddy[split]) {
      // When found, pop the split buddy.
      const auto result = free_buddy[split];
      free_buddy[split] = result->next;
      // Iterate over all resulting free splitted buddies.
      for (int i = split - 1; i >= index; --i) {
        // Push those new free splitted buddies by first computing their
        // address. We need to take care of pointer arithmetic because we are
        // counting bytes.
        free_buddy[i] =
            result + ((1 << (min_page_size_exp + i)) / sizeof(node*));
        // We know that the list was previously empty.
        free_buddy[i]->next = nullptr;
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
  if (index < 0 || index > free_buddy.size() ||
      ((page - base) * sizeof(node*) &
       ((1ull << (index + min_page_size_exp)) - 1)))
    return;
  // We assume that we are the only ones that can write into unreserved memory.
  // So we only have to check if a given page is already a free page.
  for (auto it = free_buddy[index]; it; it = it->next)
    if (it == page) return;
  // At this point, we know the given address was allocated by the buddy system.

  auto new_buddy = page;

  for (;; ++index) {
    // Construct mask and test numbers to test if two pages are buddies.
    const auto is_buddy_mask =
        ~(static_cast<intptr_t>(1) << (index + min_page_size_exp));
    const auto buddy_test = ((new_buddy - base) * sizeof(node)) & is_buddy_mask;

    // Go trough the list of pages with this size and look for the possible
    // merge buddy.
    node* merge_buddy = nullptr;
    auto prev = reinterpret_cast<node*>(&free_buddy[index]);
    for (auto it = free_buddy[index]; it; prev = it, it = it->next) {
      // Test if the current page in the list is the buddy.
      if ((((it - base) * sizeof(node)) & is_buddy_mask) == buddy_test) {
        merge_buddy = it;
        break;
      }
    }
    // If there is no buddy for merging, push the given page to the list.
    // At some point, this will always be true.
    if (merge_buddy == nullptr) {
      new_buddy->next = free_buddy[index];
      free_buddy[index] = new_buddy;
      return;
    }
    // Otherwise, we have found the buddy page and therefore have to pop and
    // merge it with our current page.
    prev->next = merge_buddy->next;
    new_buddy = base + buddy_test / sizeof(node);
    // We have to repeat these instructions to check if we have to do a merge
    // for the next higher page size.
  }
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
     << " B" << '\n'
     << '\n'
     << "base pointer offset    = " << setw(20) << bs.base << '\n'
     << "managed memory size    = " << setw(20) << bs.size << " B" << '\n'
     << "alignment base pointer = " << setw(20) << alignment_of_ptr(bs.base)
     << " B" << '\n'
     << '\n'
     << "unused memory offset   = " << setw(20)
     << decltype(bs.memory)(bs.base) - bs.memory << " B" << '\n'
     << "unused memory end      = " << setw(20)
     << (bs.memory + bs.memory_size) - (decltype(bs.memory)(bs.base) + bs.size)
     << " B" << '\n'
     << "unused memory          = " << setw(20) << bs.memory_size - bs.size
     << " B" << '\n'
     << '\n'
     << "page header size       = " << setw(20) << bs.header_size << " B"
     << '\n'
     << "page alignment         = " << setw(20) << bs.page_alignment << " B"
     << '\n'
     << "maximal page size      = " << setw(20) << bs.max_page_size << " B"
     << '\n'
     << "maximal page size exp  = " << setw(20) << bs.max_page_size_exp << '\n'
     << "minimal page size      = " << setw(20) << bs.min_page_size << " B"
     << '\n'
     << "minimal page size exp  = " << setw(20) << bs.min_page_size_exp << '\n'
     << '\n'
     << "free buddy list size   = " << setw(20) << bs.free_buddy.size() << '\n'
     << "free buddy list memory = " << setw(20)
     << bs.free_buddy.size() * sizeof(bs.free_buddy[0]) << " B" << '\n'
     << '\n'
     << "free buddy list content:" << '\n';

  for (int i = bs.max_page_size_exp; i >= bs.min_page_size_exp; --i) {
    os << setw(4) << "2^" << setw(2) << i << " B = " << setw(10) << (1ull << i)
       << " B :";
    for (auto it = bs.free_buddy[i - bs.min_page_size_exp]; it; it = it->next)
      os << setw(5) << "-->" << setw(10)
         << (it - bs.base) * sizeof(bs.free_buddy[0]) << " (" << it << ")";
    os << '\n';
  }
  return os << setfill('-') << setw(80) << '\n' << setfill(' ');
}
