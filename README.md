# UDU
**UDU** is an extremely fast, cross-platform tool for summarizing file and directory sizes by recursively scanning directories using a parallel traversal engine implemented with [OpenMP](https://www.openmp.org/), making it significantly faster than traditional tools on multi-core systems.

[![CI](https://github.com/gnualmalki/udu/actions/workflows/ci.yml/badge.svg)](https://github.com/gnualmalki/udu/actions/workflows/ci.yml)

See [Benchmarks](./BENCHMARKS.md).

## Build Instructions
building *UDU* requires a modern C compiler such as [GCC](https://gcc.gnu.org/) 9.x or later, [Clang](https://clang.llvm.org/) 14.x or later, or [MSVC](https://visualstudio.microsoft.com/) 17.x or later, along with [CMake](https://cmake.org/) 3.15 or later and [OpenMP](https://www.openmp.org/) 3.1 or later (optional: parallel processing).

**Note on Windows:** MSVC only supports OpenMP 2.0, which doesn't meet the requirements for building this program. We recommend using GCC or Clang through [MSYS2](https://www.msys2.org/) instead, though alternative Windows development environments like [Cygwin](https://www.cygwin.com/) also provide modern compiler toolchains with full OpenMP support.

#### UNIX

Clone the repository and build using these commands:

```bash
git clone --depth=1 https://github.com/gnualmalki/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
cmake --install build  # may require administrator privileges
```

#### Windows (MSYS2)

After installing MSYS2, open the UCRT64 terminal and install the required packages using `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake`, then follow the instructions above.


## Usage

```bash
Usage: udu [option(s)]... [path(s)]...
 extremely fast disk usage analyzer with parallel traversal engine.

 OPTIONS:
  -a, --apparent-size    show file sizes instead of disk usage
                          (apparent = bytes reported by filesystem,
                           disk usage = actual space allocated)
  -h, --help             display this help and exit
  -q, --quiet            display output at program exit (default)
  -v, --verbose          display each processed file
      --version          display version info and exit
  -X, --exclude=PATTERN  skip files or directories that match glob pattern
                          *        any characters
                          ?        single character
                          [abc]    any char in set
                          Examples: '*.log', 'temp?', '[0-9]*'

 EXAMPLE:
   udu ~/ -avX epstein-files

Report bugs to <https://github.com/makestatic/udu/issues>
```


## License
This program is distributed under GPL-3.0 or later, see the [LICENSE](./LICENSE) file for details.


### Acknowledgements
- Thanks to the [OpenMP](https://openmp.org) specification and its implementations by [GCC](https://gcc.gnu.org/) and [LLVM](https://llvm.org/) for making FOSS high-performance parallel computing possible.
