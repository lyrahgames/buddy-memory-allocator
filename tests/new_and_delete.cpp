#include <buddy_memory_allocator/buddy_memory_allocator.hpp>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

using namespace std;

int main() {
  mt19937 rng{random_device{}()};

  buddy_memory_allocator allocator{size_t{1} << 12};

  string input{};
  while (getline(cin, input)) {
    int *tmp = new (allocator) int[20];
    *tmp = rng();
    cout << setw(20) << tmp << ":" << setw(20) << *tmp << '\n' << allocator;
    operator delete[](tmp, allocator);
  }
}