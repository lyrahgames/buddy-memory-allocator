# Buddy Memory Allocator

A simple example implementation of the [buddy memory system](https://en.wikipedia.org/wiki/Buddy_memory_allocation).
It was put into an easy-to-use C++ header-only library curently based on the [build2](https://build2.org/) build system.

![](https://img.shields.io/github/languages/top/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge)
![](https://img.shields.io/github/languages/code-size/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge)
![](https://img.shields.io/github/repo-size/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge)
![](https://img.shields.io/github/license/lyrahgames/buddy-memory-allocator.svg?style=for-the-badge&color=blue)

<b>
<table>
    <tr>
        <td>
            master
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
            Current
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
        <!-- <td>
            <img src="https://img.shields.io/github/tag/lyrahgames/buddy-memory-allocator.svg?logo=github&logoColor=white">
        </td>
        <td>
            <img src="https://img.shields.io/github/tag-date/lyrahgames/buddy-memory-allocator.svg?label=latest%20tag&logo=github&logoColor=white">
        </td> -->
    </tr>
</table>
</b>

## Author
- Markus Pawellek (markus.pawellek@mailbox.org)

## Requirements

- C++17
- [build2](https://build2.org/) (or some manual work for other build systems)
- x86 or x86_64 architecture (support for the [`bsr`](https://c9x.me/x86/html/file_module_x86_id_20.html) instruction)

## Supported Platforms

- Operating System: Linux
- Compiler: GCC | Clang
- Build System: [build2](https://build2.org/)

## Installation

## Usage with build2

`manifest`:
Add the following entry to the `manifest` file.

    depends: buddy-memory-allocator

`buildfile`:
Add these entries to your `buildfile`.

    import libs = buddy-memory-allocator%lib{buddy-memory-allocator}
    exe{your-executable}: {hxx cxx}{**} $libs

## Example

```c++
#include <buddy_memory_allocator/buddy_memory_allocator.hpp>
int main(){
    // Construct the allocator and reserve 2 GiB to manage.
    buddy_memory_arena arena{size_t{1} << 31};
    // Allocate actual memory in bytes.
    void* ptr = arena.malloc(123);
    // Free the memory after usage by providing the pointer.
    arena.free(ptr);
}
```

## API

### Constructor
```c++
    buddy_memory_arena::buddy_memory_arena(size_t s);
```

### Bare-Bones Allocation Member Function
```c++
    void* buddy_memory_arena::malloc(size_t size) noexcept;
```

### Bare-Bones Deallocation Member Function
```c++
    void buddy_memory_arena::free(void* address) noexcept;
```

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