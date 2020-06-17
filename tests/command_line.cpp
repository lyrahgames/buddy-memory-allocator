#include <buddy_system/buddy_system.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std;

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
  uniform_real_distribution<float> alloc_or_free_dist{};

  unordered_map<void*, size_t> allocation_table{};

  buddy_system::arena arena{size};

  cout << '\n' << arena << '\n';

  prompt();
  string cmd;
  string arg;
  while (cin >> cmd) {
    getline(std::cin, arg);

    if (cmd == "help") {
      cout << '\n';
      separate();
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
           << '\n';
      separate();
      cout << '\n';
    } else if (cmd == "q" || cmd == "quit")
      break;
    else if (cmd == "arena") {
      cout << '\n' << arena << '\n';
    } else if (cmd == "list") {
      cout << '\n';
      separate();
      cout << setw(20) << "address" << setw(15) << "index" << setw(20)
           << "size [B]" << setw(20) << "page size [B]" << '\n';
      separate();
      for (auto [p, n] : allocation_table)
        cout << setw(20) << p << setw(15) << arena.index_of_node_ptr(p)
             << setw(20) << n << setw(20) << arena.page_size(p) << '\n';
      separate();
      cout << '\n';
    } else if (cmd == "malloc") {
      size_t size;
      stringstream tmp{arg};
      tmp >> size;
      const auto p = arena.malloc(size);
      cout << '\n';
      separate();
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
      separate();
      cout << '\n';
    } else if (cmd == "free") {
      size_t index;
      stringstream tmp{arg};
      tmp >> index;
      cout << '\n';
      separate();
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
      separate();
      cout << '\n';
    } else {
      cout << "\nUnknown command. Use 'help' to print the help message.\n\n";
    }

    //   if ((allocation_table.size() == 0) ||
    //       alloc_or_free_dist(rng) < 0.6f) {
    //     uniform_int_distribution<int> mem_exp_dist{4, 31};
    //     const auto mem_exp = mem_exp_dist(rng);
    //     const auto mem_size = (size_t{1} << mem_exp) - 8;

    //     cout << "allocate " << mem_size << " B:\n";

    //     const auto p = arena.malloc(mem_size);
    //     if (!p) {
    //       cout << "unsuccessful\n";
    //     } else {
    //       allocation_table.insert(p);
    //       cout << "success: p = " << arena.index_of_node_ptr(p) << " (" << p
    //            << ")" << '\n'
    //            << "page size = " << arena.page_size(p) << '\n'
    //            << "alignment = " << buddy_system::alignment_of_ptr(p) <<
    //            '\n';
    //     }
    //   } else {
    //     uniform_int_distribution<size_t> pointer_dist{
    //         0, allocation_table.size() - 1};
    //     const auto index = pointer_dist(rng);

    //     auto it = allocation_table.begin();
    //     advance(it, index);

    //     cout << "deallocate " << arena.index_of_node_ptr(*it) << " (" << *it
    //          << ")" << '\n';

    //     arena.free(*it);
    //     allocation_table.erase(it);
    //   }

    //   cout << arena;
    prompt();
  }
}