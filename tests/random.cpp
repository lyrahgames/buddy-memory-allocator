#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_set>
//
#include <lyrahgames/buddy_system/buddy_system.hpp>

using namespace std;
using namespace lyrahgames;

int main() {
  mt19937 rng{random_device{}()};
  uniform_real_distribution<float> alloc_or_free_dist{};

  unordered_set<void*> pointer_to_allocations{};

  buddy_system::arena arena{size_t{1} << 32};  // 4 GiB
  cout << "initial\n" << arena << '\n';

  std::string s;
  while (std::getline(std::cin, s)) {
    if ((pointer_to_allocations.size() == 0) ||
        alloc_or_free_dist(rng) < 0.6f) {
      uniform_int_distribution<int> mem_exp_dist{4, 31};
      const auto mem_exp = mem_exp_dist(rng);
      const auto mem_size = (size_t{1} << mem_exp) - 8;

      cout << "allocate " << mem_size << " B:\n";

      const auto p = arena.malloc(mem_size);
      if (!p) {
        cout << "unsuccessful\n";
      } else {
        pointer_to_allocations.insert(p);
        cout << "success: p = " << arena.index_of_node_ptr(p) << " (" << p
             << ")" << '\n'
             << "page size = " << arena.page_size(p) << '\n'
             << "alignment = " << buddy_system::alignment_of_ptr(p) << '\n';
      }
    } else {
      uniform_int_distribution<size_t> pointer_dist{
          0, pointer_to_allocations.size() - 1};
      const auto index = pointer_dist(rng);

      auto it = pointer_to_allocations.begin();
      advance(it, index);

      cout << "deallocate " << arena.index_of_node_ptr(*it) << " (" << *it
           << ")" << '\n';

      arena.free(*it);
      pointer_to_allocations.erase(it);
    }

    cout << arena;
  }
}