#include <buddy_system/buddy_system.hpp>
#include <iomanip>
#include <iostream>
#include <list>
#include <vector>

using namespace std;

template <typename T>
using my_vector = vector<T, buddy_system::allocator<T>>;
template <typename T>
using my_list = list<T, buddy_system::allocator<T>>;

template <typename T>
ostream& operator<<(ostream& os, const my_vector<T>& v) {
  for (const auto x : v) os << setw(10) << x;
  return os << '\n';
}

template <typename T>
ostream& operator<<(ostream& os, const my_list<T>& v) {
  for (const auto x : v) os << setw(10) << x;
  return os << '\n';
}

int main() {
  buddy_system::arena arena{size_t{1} << 12};  // 4096 B
  cout << arena;

  string input{};
  getline(cin, input);
  {
    my_vector<int> v1{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                      arena};  // buddy_system_arena can be implicitly casted to
                               // buddy_system_allocator
    cout << v1 << arena;
    getline(cin, input);

    my_vector<int> v2{{10, 20, 30, 40, 50, 60}, arena};
    cout << v1 << v2 << arena;
    getline(cin, input);

    v1.clear();
    v1.shrink_to_fit();
    cout << v1 << v2 << arena;
    getline(cin, input);

    v2.push_back(70);
    v2.push_back(80);
    v2.push_back(90);
    v2.push_back(100);
    v2.push_back(110);
    v2.push_back(120);
    v2.push_back(130);
    v2.push_back(140);
    v2.push_back(150);
    v2.push_back(160);
    cout << v1 << v2 << arena;
    getline(cin, input);
  }
  cout << arena;
  getline(cin, input);
  {
    my_list<int> v1{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                    arena};  // buddy_system_arena can be implicitly
                             // casted to buddy_system_allocator
    cout << v1 << arena;
    getline(cin, input);

    my_list<int> v2{{10, 20, 30, 40, 50, 60}, arena};
    cout << v1 << v2 << arena;
    getline(cin, input);

    v1.clear();
    cout << v1 << v2 << arena;
    getline(cin, input);

    v2.push_back(70);
    v2.push_back(80);
    v2.push_back(90);
    v2.push_back(100);
    v2.push_back(110);
    v2.push_back(120);
    v2.push_back(130);
    v2.push_back(140);
    v2.push_back(150);
    v2.push_back(160);
    cout << v1 << v2 << arena;
    getline(cin, input);
  }
  cout << arena;
  getline(cin, input);
}