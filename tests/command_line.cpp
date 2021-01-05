#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
//
#include <lyrahgames/buddy_system/buddy_system.hpp>

using namespace std;
using namespace lyrahgames;

inline void prompt() { cout << "$ >>> " << flush; }
inline void separate() {
  cout << setfill('-') << setw(80) << '\n' << setfill(' ');
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cout << "usage:\n" << argv[0] << " <buddy system memory size in bytes>\n";
    return 0;
  }

  size_t size;
  stringstream input{argv[1]};
  input >> size;

  mt19937 rng{random_device{}()};
  unordered_map<void*, size_t> allocation_table{};
  buddy_system::arena arena{size};

  prompt();
  string cmd;
  string arg;
  while (cin >> cmd) {
    getline(std::cin, arg);

    cout << '\n';
    separate();

    if (cmd == "help") {
      cout << "Help Message\n";
      separate();
      cout << setw(20) << "help" << setfill('.') << setw(10) << '.'
           << setfill(' ') << "Show this help." << '\n'
           << setw(20) << "quit" << setfill('.') << setw(10) << '.'
           << setfill(' ') << "Quit the command line." << '\n'
           << setw(20) << "arena" << setfill('.') << setw(10) << '.'
           << setfill(' ') << "Print arena state." << '\n'
           << setw(20) << "list" << setfill('.') << setw(10) << '.'
           << setfill(' ') << "Print allocation table." << '\n'
           << setw(20) << "malloc <size>" << setfill('.') << setw(10) << '.'
           << setfill(' ') << "Allocate memory block of at least 'size' B."
           << '\n'
           << setw(20) << "free <index>" << setfill('.') << setw(10) << '.'
           << setfill(' ') << "Deallocate memory block with index 'index'."
           << '\n'
           << setw(20) << "random" << setfill('.') << setw(10) << '.'
           << setfill(' ') << "Randomly allocate or deallocate memory blocks."
           << '\n';
    } else if (cmd == "q" || cmd == "quit")
      break;
    else if (cmd == "arena") {
      cout << '\n' << arena << '\n';
    } else if (cmd == "list") {
      cout << setw(20) << "address" << setw(15) << "index" << setw(20)
           << "size [B]" << setw(20) << "page size [B]" << '\n';
      separate();
      for (auto [p, n] : allocation_table)
        cout << setw(20) << p << setw(15) << arena.index_of_node_ptr(p)
             << setw(20) << n << setw(20) << arena.page_size(p) << '\n';
    } else if (cmd == "malloc") {
      size_t size;
      stringstream tmp{arg};
      tmp >> size;
      const auto p = arena.malloc(size);
      if (!p) {
        cout << "Memory allocation with " << size << " B was unsuccessful.\n"
             << "No remaining free page is large enough.\n";
      } else {
        allocation_table[p] = size;
        cout << "Memory allocation with size " << size
             << " B was successful.\n\n"
             << setw(20) << "address = " << p << '\n'
             << setw(20) << "index = " << arena.index_of_node_ptr(p) << '\n'
             << setw(20) << "size = " << size << " B" << '\n'
             << setw(20) << "page size = " << arena.page_size(p) << " B" << '\n'
             << setw(20) << "alignment = " << buddy_system::alignment_of_ptr(p)
             << " B" << '\n';
      }
    } else if (cmd == "free") {
      size_t index;
      stringstream tmp{arg};
      tmp >> index;
      const auto p = arena.void_ptr_of_index(index);
      const auto it = allocation_table.find(p);
      if (it != allocation_table.end()) {
        arena.free(p);
        allocation_table.erase(it);
        cout << "Deallocated memory block with index " << index << ".\n";
      } else {
        cout << "Cannot deallocate memory block with given index " << index
             << ".\n";
      }
    } else if (cmd == "random") {
      if ((allocation_table.size() == 0) ||
          uniform_real_distribution<float>{}(rng) < 0.6f) {
        uniform_int_distribution<size_t> mem_exp_dist{
            0, buddy_system::log2(arena.max_page_size() - 1) + 1};
        const auto mem_exp = mem_exp_dist(rng);
        auto mem_size = size_t{1} << mem_exp;
        mem_size += uniform_int_distribution<size_t>{0, mem_size - 1}(rng);
        cout << "Randomly allocate memory of size " << mem_size << " B:\n";
        const auto p = arena.malloc(mem_size);
        if (!p) {
          cout << "Memory allocation with " << mem_size
               << " B was unsuccessful.\n"
               << "No remaining free page is large enough.\n";
        } else {
          allocation_table[p] = mem_size;
          cout << "Memory allocation with size " << mem_size
               << " B was successful.\n\n"
               << setw(20) << "address = " << p << '\n'
               << setw(20) << "index = " << arena.index_of_node_ptr(p) << '\n'
               << setw(20) << "size = " << mem_size << " B" << '\n'
               << setw(20) << "page size = " << arena.page_size(p) << " B"
               << '\n'
               << setw(20)
               << "alignment = " << buddy_system::alignment_of_ptr(p) << " B"
               << '\n';
        }
      } else {
        uniform_int_distribution<size_t> pointer_dist{
            0, allocation_table.size() - 1};
        const auto index = pointer_dist(rng);
        auto it = allocation_table.begin();
        advance(it, index);
        cout << "Randomly deallocate memory block with index "
             << arena.index_of_node_ptr(it->first) << " (" << it->first << ")"
             << '\n';
        arena.free(it->first);
        allocation_table.erase(it);
      }
    } else {
      cout << "\nUnknown command. Use 'help' to print the help message.\n\n";
    }

    separate();
    cout << '\n';

    prompt();
  }
}