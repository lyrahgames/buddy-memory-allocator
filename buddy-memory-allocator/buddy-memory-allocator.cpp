#include <array>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace std;

struct buddy_memory_allocator {
  static constexpr int min_exp = 6;
  static constexpr int max_exp = 10;
  static constexpr int offset = 16;

  struct node {
    node* next;
  };
  static constexpr int node_size_exp = 3;

  // using pointer_type = char*;
  using pointer_type = node*;
  using result_type = void*;
  pointer_type base;
  pointer_type free_buddy[max_exp - min_exp + 1]{};
  vector<byte> memory;

  buddy_memory_allocator() : memory(1 << max_exp) {
    base = reinterpret_cast<pointer_type>(memory.data());
    free_buddy[max_exp - min_exp] = base;
    free_buddy[max_exp - min_exp]->next = nullptr;
  }

  result_type allocate(int expo) {
    if (max_exp < expo || expo < min_exp) return nullptr;
    const int index = expo - min_exp;
    int split_index = 0;
    for (int i = index; i <= max_exp - min_exp; ++i) {
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
      free_buddy[i] = result + (1 << (min_exp - node_size_exp + i));
      free_buddy[i]->next = nullptr;
    }

    return reinterpret_cast<result_type>(result);
  }

  void deallocate(result_type address, int expo) {
    int index = expo - min_exp;
    auto new_buddy = reinterpret_cast<pointer_type>(address);

    while (true) {
      const auto is_buddy_mask =
          ~(static_cast<intptr_t>(1) << (index + min_exp));
      const auto buddy_test =
          ((new_buddy - base) * sizeof(node)) & is_buddy_mask;
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

    // push new buddy to list
    // new_buddy->next = free_buddy[index + 1];
    // free_buddy[index + 1] = new_buddy;

    // cout << "merge buddy found\n"
    //      << (merge_buddy - base) * sizeof(node) << '\n'
    //      << merge_buddy->next << '\n'
    //      << (merge_buddy->next - base) * sizeof(node) << '\n'
    //      << (prev - base) * sizeof(node) << '\n'
    //      << (prev->next - base) * sizeof(node) << '\n'
    //      << buddy_test << '\n'
    //      << (new_buddy - base) * sizeof(node) << '\n'
    //      << flush;
  }
};

ostream& operator<<(ostream& os, const buddy_memory_allocator& bs) {
  os << "offset of buddy memory = "
     << reinterpret_cast<const void*>(bs.memory.data()) << '\n'
     << "size of buddy memory   = " << bs.memory.size() << '\n';
  for (int i = bs.max_exp; i >= bs.min_exp; --i) {
    os << setw(4) << i << ":";
    for (auto it = bs.free_buddy[i - bs.min_exp]; it; it = it->next)
      os << setw(10) << (reinterpret_cast<byte*>(it) - bs.memory.data());
    os << '\n';
  }
  return os;
}

int main() {
  buddy_memory_allocator system{};
  cout << system << '\n';
  auto p1 = system.allocate(8);
  cout << "p1 = system.allocate(8) = " << p1 << "\n" << system << '\n';

  auto p2 = system.allocate(7);
  cout << "p2 = system.allocate(7) = " << p2 << "\n" << system << '\n';

  auto p3 = system.allocate(8);
  cout << "p3 = system.allocate(8) = " << p3 << "\n" << system << '\n';

  system.deallocate(p2, 7);
  cout << "system.deallocate(p2, 7)\n" << system << '\n';

  system.deallocate(p1, 8);
  cout << "system.deallocate(p1, 8)\n" << system << '\n';

  system.deallocate(p3, 8);
  cout << "system.deallocate(p3, 8)\n" << system << '\n';
}