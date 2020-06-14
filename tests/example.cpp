#include <buddy_memory_allocator/buddy_memory_allocator.hpp>
#include <iomanip>
#include <iostream>

using namespace std;

int main() {
  buddy_memory_allocator allocator{size_t{1} << 10};  // 1 KiB
  cout << "initial\n" << allocator << '\n';

  auto p1 = allocator.malloc(223);
  cout << "malloc(223): p1 = " << allocator.index_of_node_ptr(p1) << " (" << p1
       << ")"
       << "\n"
       << "page size = " << allocator.page_size(p1) << " B" << '\n'
       << "alignment_of_ptr(p1) = " << alignment_of_ptr(p1) << '\n'
       << allocator << '\n';

  auto p2 = allocator.malloc(120);
  cout << "malloc(120): p2 = " << allocator.index_of_node_ptr(p2) << " (" << p2
       << ")"
       << "\n"
       << "page size = " << allocator.page_size(p2) << " B" << '\n'
       << "alignment_of_ptr(p2) = " << alignment_of_ptr(p2) << '\n'
       << allocator << '\n';

  auto p3 = allocator.malloc(128);
  cout << "malloc(128): p3 = " << allocator.index_of_node_ptr(p3) << " (" << p3
       << ")"
       << "\n"
       << "page size = " << allocator.page_size(p3) << " B" << '\n'
       << "alignment_of_ptr(p3) = " << alignment_of_ptr(p3) << '\n'
       << allocator << '\n';

  // allocator.deallocate(p2, 7);
  allocator.free(p2);
  cout << "free(p2 {" << p2 << "}):\n" << allocator << '\n';

  // allocator.deallocate(p1, 8);
  allocator.free(p1);
  cout << "free(p1 {" << p1 << "}):\n" << allocator << '\n';

  // allocator.deallocate(p3, 8);
  allocator.free(p3);
  cout << "free(p3 {" << p3 << "}):\n" << allocator << '\n';

  cout << "sizeof(size_t) = " << sizeof(size_t) << '\n'
       << "sizeof(ptrdiff_t) = " << sizeof(ptrdiff_t) << '\n'
       << "sizeof(intptr_t) = " << sizeof(intptr_t) << '\n'
       << "sizeof(uintptr_t) = " << sizeof(uintptr_t) << '\n';
}