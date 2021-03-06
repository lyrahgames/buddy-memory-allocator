#include <iomanip>
#include <iostream>
#include <random>
#include <string>
//
#include <lyrahgames/buddy_system/buddy_system.hpp>

using namespace std;
using namespace lyrahgames;

struct test {
  test() { cout << "Constructor called.\n"; }
  ~test() { cout << "Destructor called.\n"; }
  int data{};
};

int main() {
  mt19937 rng{random_device{}()};

  buddy_system::arena arena{size_t{1} << 12};

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