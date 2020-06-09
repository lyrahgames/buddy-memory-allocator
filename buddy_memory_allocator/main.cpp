#include <buddy_memory_allocator/buddy_memory_allocator.hpp>
#include <iomanip>
#include <iostream>

using namespace std;

int main() {
  buddy_memory_allocator system{1ull << 31};  // 2 GiB
  cout << "initial\n" << system << '\n';

  // auto p1 = system.allocate(8);
  auto p1 = system.malloc(223);
  cout << "malloc(223): p1 = " << p1 << "\n"
       << "alignment_of_ptr(p1) = " << alignment_of_ptr(p1) << '\n'
       << system << '\n';

  // auto p2 = system.allocate(7);
  auto p2 = system.malloc(120);
  cout << "malloc(120): p2 = " << p2 << "\n"
       << "alignment_of_ptr(p2) = " << alignment_of_ptr(p2) << '\n'
       << system << '\n';

  // auto p3 = system.allocate(8);
  auto p3 = system.malloc(128);
  cout << "malloc(128): p3 = " << p3 << "\n"
       << "alignment_of_ptr(p3) = " << alignment_of_ptr(p3) << '\n'
       << system << '\n';

  // system.deallocate(p2, 7);
  system.free(p2);
  cout << "free(p2 {" << p2 << "}):\n" << system << '\n';

  // system.deallocate(p1, 8);
  system.free(p1);
  cout << "free(p1 {" << p1 << "}):\n" << system << '\n';

  // system.deallocate(p3, 8);
  system.free(p3);
  cout << "free(p3 {" << p3 << "}):\n" << system << '\n';
}