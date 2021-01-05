# Buddy Memory System

A simple example implementation of the [buddy memory system](https://en.wikipedia.org/wiki/Buddy_memory_allocation).
It was put into an easy-to-use C++ header-only library curently based on the [build2](https://build2.org/) build system.

![](https://img.shields.io/github/languages/top/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge)
![](https://img.shields.io/github/languages/code-size/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge)
![](https://img.shields.io/github/repo-size/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge)
![](https://img.shields.io/github/license/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge&color=blue)

<table>
    <tr>
        <td>
            <strong>master</strong>
        </td>
        <td>
            <a href="https://github.com/lyrahgames/buddy-memory-allocator">
                <img src="https://img.shields.io/github/last-commit/lyrahgames/buddy-memory-allocator/master.svg?logo=github&logoColor=white">
            </a>
        </td>    
        <!-- <td>
            <a href="https://circleci.com/gh/lyrahgames/buddy-memory-allocator/tree/master"><img src="https://circleci.com/gh/lyrahgames/buddy-memory-allocator/tree/master.svg?style=svg"></a>
        </td>
        <td>
            <a href="https://codecov.io/gh/lyrahgames/buddy-memory-allocator">
              <img src="https://codecov.io/gh/lyrahgames/buddy-memory-allocator/branch/master/graph/badge.svg" />
            </a>
        </td> -->
    </tr>
    <!-- <tr>
        <td>
            develop
        </td>
        <td>
            <a href="https://github.com/lyrahgames/buddy-memory-allocator/tree/develop">
                <img src="https://img.shields.io/github/last-commit/lyrahgames/buddy-memory-allocator/develop.svg?logo=github&logoColor=white">
            </a>
        </td>    
        <td>
            <a href="https://circleci.com/gh/lyrahgames/buddy-memory-allocator/tree/develop"><img src="https://circleci.com/gh/lyrahgames/buddy-memory-allocator/tree/develop.svg?style=svg"></a>
        </td>
        <td>
            <a href="https://codecov.io/gh/lyrahgames/buddy-memory-allocator">
              <img src="https://codecov.io/gh/lyrahgames/buddy-memory-allocator/branch/develop/graph/badge.svg" />
            </a>
        </td>
    </tr> -->
    <tr>
        <td>
        </td>
    </tr>
    <tr>
        <td>
            <strong>Current</strong>
        </td>
        <td>
            <a href="https://github.com/lyrahgames/buddy-memory-allocator">
                <img src="https://img.shields.io/github/commit-activity/y/lyrahgames/buddy-memory-allocator.svg?logo=github&logoColor=white">
            </a>
        </td>
        <!-- <td>
            <img src="https://img.shields.io/github/release/lyrahgames/buddy-memory-allocator.svg?logo=github&logoColor=white">
        </td>
        <td>
            <img src="https://img.shields.io/github/release-pre/lyrahgames/buddy-memory-allocator.svg?label=pre-release&logo=github&logoColor=white">
        </td> -->
        <td>
            <img src="https://img.shields.io/github/tag/lyrahgames/buddy-memory-allocator.svg?logo=github&logoColor=white">
        </td>
        <td>
            <img src="https://img.shields.io/github/tag-date/lyrahgames/buddy-memory-allocator.svg?label=latest%20tag&logo=github&logoColor=white">
        </td>
    </tr>
</table>

## Author
- Markus Pawellek "lyrahgames" (lyrahgames@mailbox.org)

## Requirements
- C++17
- [build2](https://build2.org/) (or some manual work for other build systems)
- x86 or x86_64 architecture (support for the [`bsr`](https://c9x.me/x86/html/file_module_x86_id_20.html) instruction)

## Tested Platforms
- Operating System: Linux
- Compiler: GCC | Clang
- Build System: [build2](https://build2.org/)

## Usage with build2
Add this repository to the `repositories.manifest` file of your build2 package.

    :
    role: prerequisite
    location: https://github.com/lyrahgames/buddy-system.git

Add the following entry to the `manifest` file with a possible version dependency.

    depends: lyrahgames-buddy-system

Add these entries to your `buildfile`.

    import libs = lyrahgames-buddy-system%lib{lyrahgames-buddy-system}
    exe{your-executable}: {hxx cxx}{**} $libs


## Installation
The standard installation process will only install the header-only library.
If you are interested in installing examples, benchmarks, or tests, you have to run their installation commands manually.

    bpkg -d build2-packages cc \
      config.install.root=/usr/local \
      config.install.sudo=sudo

Get the latest package release and build it.

    bpkg build https://github.com/lyrahgames/buddy-system.git

Install the built package.

    bpkg install lyrahgames-buddy-system

For uninstalling, do the following.

    bpkg uninstall lyrahgames-buddy-system

If your package uses an explicit `depends: lyrahgames-buddy-system` make sure to initialize this dependency as a system dependency when creating a new configuration.

    bdep init -C @build cc config.cxx=g++ "config.cxx.coptions=-O3" -- "?sys:lyrahgames-buddy-system/*"

## Example
### Bare-Bones Malloc and Free Example
```c++
#include <lyrahgames/buddy_system/buddy_system.hpp>

using namespace lyrahgames;

int main(){
    // Construct the allocator and reserve 2 GiB to manage.
    buddy_system::arena arena{size_t{1} << 31};
    // Allocate actual memory in bytes.
    void* ptr = arena.malloc(123);
    // Free the memory after usage by providing the pointer.
    arena.free(ptr);
}
```
### C++ Containers with Buddy System Allocator
```c++
#include <vector>
//
#include <lyrahgames/buddy_system/buddy_system.hpp>

using namespace std;
using namespace lyrahgames;

template <typename T>
using my_vector = vector<T, buddy_system::allocator<T>>;

int main() {
  buddy_system::arena arena{size_t{1} << 12};  // 4096 B
  my_vector<int> v{{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   arena};
}
```

## API

### Constructor
```c++
    lyrahgames::buddy_system::arena::arena(size_t s);
```

### Bare-Bones Allocation Member Function
```c++
    void* lyrahgames::buddy_system::arena::malloc(size_t size) noexcept;
```
This function returns `nullptr` if allocation was not successful.
Every pointer returned by a successful memory allocation is at least 64-byte aligned.

### Throwing Allocation Member Function
```c++
    void* lyrahgames::buddy_system::arena::allocate(size_t size);
```
This functions throws `std::bad_alloc` if allocation was not successful.
Otherwise, this function is the same as `buddy_system::arena::malloc`.

### Bare-Bones Deallocation Member Function
```c++
    void lyrahgames::buddy_system::arena::free(void* address) noexcept;
    void lyrahgames::buddy_system::arena::deallocate(void* address) noexcept;
```
These functions do the same and do not throw any exception.
If the given pointer was not allocated before by the system, nothing should happen.

## Features

- low memory consumption
- fast allocation
- expectation of fast deallocation
- returned pointers are at least 64-byte aligned

## Background

## Implementation Details

- no binary tree used
- allocation sizes will be rounded to the next power of two together with page header
- every page contains an 8-byte header with its size
- table of one-directional linked lists stored with the use of the free memory blocks so no other dynamic memory management is needed

## References
- https://en.wikipedia.org/wiki/Buddy_memory_allocation
- https://www.geeksforgeeks.org/buddy-memory-allocation-program-set-2-deallocation/
- https://github.com/evanw/buddy-malloc
- https://github.com/lotabout/buddy-system
- https://github.com/sdpetrides/BuddyAllocator

## Further Reading
- https://sourceware.org/glibc/wiki/MallocInternals
- https://man7.org/linux/man-pages/man2/brk.2.html