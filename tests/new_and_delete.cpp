#include <buddy_memory_allocator/buddy_memory_allocator.hpp>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>

using namespace std;

struct test {
  test() { cout << "Constructor called.\n"; }
  ~test() { cout << "Destructor called.\n"; }
  int data{};
};

int main() {
  mt19937 rng{random_device{}()};

  buddy_memory_arena arena{size_t{1} << 12};

  string input{};
  while (getline(cin, input)) {
    auto tmp = new (arena) test[10];
    // tmp = rng();
    // cout << setw(20) << tmp << ":" << setw(20) << *tmp << '\n' << arena;

    // Manually call the constructor.
    for (size_t i = 0; i < 10; ++i) (tmp + i)->~test();
    // And then free the memory.
    operator delete[](tmp, arena);
  }
}