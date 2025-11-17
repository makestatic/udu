# UDU

UDU is an extremely fast and cross-platform tool for measuring file and directory sizes, using a parallel traversal engine implemented with [OpenMP](https://www.openmp.org/) to recursively scan directories, making it significantly faster than traditional tools on multi-core systems.

See [Benchmarks](./BENCHMARKS.md) for performance comparisons.

## Quick Install

Working on it!

## Build from Source

### C Implementation

The C implementation provides the best performance when built with OpenMP, requiring a modern C compiler such as [GCC](https://gcc.gnu.org/) 9 or later, [Clang](https://clang.llvm.org/) 14 or later, or [MSVC](https://visualstudio.microsoft.com/) 17 or later, along with [CMake](https://cmake.org/) 3.15 or later and [OpenMP](https://www.openmp.org/) 3.1 or later (optional but recommended).

**Note on Windows:** MSVC only supports OpenMP 2.0, which doesn't meet the requirements for building this program. We recommend using GCC or Clang through [MSYS2](https://www.msys2.org/) instead, though alternative Windows development environments like [Cygwin](https://www.cygwin.com/) also provide modern compiler toolchains with full OpenMP support.

#### UNIX Build Instructions

Clone the repository and build using these commands:

```bash
git clone --depth=1 https://github.com/makestatic/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
cmake --install build  # may require administrator privileges
```

#### MSYS2 Build Instructions

After installing MSYS2, open the UCRT64 terminal and install the required packages using `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake`, then clone the repository and build:

```bash
git clone --depth=1 https://github.com/makestatic/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
cmake --install build  # may require administrator privileges
```

### Zig Implementation

The Zig implementation offers easier cross-platform builds while maintaining good performance, requiring only [Zig](https://ziglang.org/) 0.15.2 installed on your system. Clone the repository if you haven't already, then navigate to the `zig` directory and run the build command:

```bash
zig build -Doptimize=ReleaseFast
```

## Usage

```bash
udu <path> [-ex=<name|path>] [-sb|--size-apparent] [-v|--verbose] [-q|--quiet] [-h|--help] [--version]
```


## License

This program is distributed under the GNU General Public License version 3, see the [LICENSE](./LICENSE) file for details.

## Acknowledgements
- Thanks to the [OpenMP](https://openmp.org) specification and its implementations by [GCC](https://gcc.gnu.org/) and [LLVM](https://llvm.org/) for making FOSS high-performance parallel computing possible.  
- Thanks to [xxHash](https://github.com/Cyan4973/xxHash) for fast hashing.
