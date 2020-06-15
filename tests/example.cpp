#include <buddy_memory_allocator/buddy_memory_allocator.hpp>
#include <iomanip>
#include <iostream>

using namespace std;

int main() {
  buddy_memory_arena arena{size_t{1} << 10};  // 1 KiB
  cout << "initial\n" << arena << '\n';

  auto p1 = arena.malloc(223);
  cout << "malloc(223): p1 = " << arena.index_of_node_ptr(p1) << " (" << p1
       << ")"
       << "\n"
       << "page size = " << arena.page_size(p1) << " B" << '\n'
       << "alignment_of_ptr(p1) = " << alignment_of_ptr(p1) << '\n'
       << arena << '\n';

  auto p2 = arena.malloc(120);
  cout << "malloc(120): p2 = " << arena.index_of_node_ptr(p2) << " (" << p2
       << ")"
       << "\n"
       << "page size = " << arena.page_size(p2) << " B" << '\n'
       << "alignment_of_ptr(p2) = " << alignment_of_ptr(p2) << '\n'
       << arena << '\n';

  auto p3 = arena.malloc(128);
  cout << "malloc(128): p3 = " << arena.index_of_node_ptr(p3) << " (" << p3
       << ")"
       << "\n"
       << "page size = " << arena.page_size(p3) << " B" << '\n'
       << "alignment_of_ptr(p3) = " << alignment_of_ptr(p3) << '\n'
       << arena << '\n';

  arena.free(p2);
  cout << "free(p2 {" << p2 << "}):\n" << arena << '\n';

  arena.free(p1);
  cout << "free(p1 {" << p1 << "}):\n" << arena << '\n';

  arena.free(p3);
  cout << "free(p3 {" << p3 << "}):\n" << arena << '\n';

  cout << "sizeof(size_t) = " << sizeof(size_t) << '\n'
       << "sizeof(ptrdiff_t) = " << sizeof(ptrdiff_t) << '\n'
       << "sizeof(intptr_t) = " << sizeof(intptr_t) << '\n'
       << "sizeof(uintptr_t) = " << sizeof(uintptr_t) << '\n';
}