# UDU

**UDU** is an extremely fast, cross-platform tool for summarizing file and directory sizes by recursively scanning directories using a parallel traversal engine implemented with [OpenMP](https://www.openmp.org/), making it significantly faster than traditional tools on multi-core systems.

See [Benchmarks](./BENCHMARKS.md) for performance comparisons.


## Architecture
UDU uses a parallel directory traversal engine to compute file and directory sizes efficiently. The master thread starts at the given path and spawns worker threads for each subdirectory. Each worker recursively enumerates its directory, computing file sizes directly and spawning further threads for nested directories. At the end the results propagate back to the master thread for aggregation.


```

         +----------------+
         | MASTER THREAD  |
         |     <path>     |
         +--------+-------+
                  |
      -------------------------
      |           |           |
+-----v-----+ +---v-----+ +---v-----+
| FILE      | | DIR A   | | DIR B   |
| (compute) | | (spawn) | | (spawn) |
+-----------+ +---------+ +---------+
                  |           |
          -------------------------
          |           |           |
      +---v---+   +---v---+   +---v---+
      | FILE  |   | DIR C |   | DIR D |
      +-------+   +-------+   +-------+
                  |           |
             +----v-----+ +---v------+
             | WORKER C | | WORKER D |
             +----------+ +----------+
                  |           |
                 ...         ...

```

## Build from Source

### C Implementation

The C implementation provides the best performance when built with OpenMP, requiring a modern C compiler such as [GCC](https://gcc.gnu.org/) 9 or later, [Clang](https://clang.llvm.org/) 14 or later, or [MSVC](https://visualstudio.microsoft.com/) 17 or later, along with [CMake](https://cmake.org/) 3.15 or later and [OpenMP](https://www.openmp.org/) 3.1 or later (optional but recommended).

**Note on Windows:** MSVC only supports OpenMP 2.0, which doesn't meet the requirements for building this program. We recommend using GCC or Clang through [MSYS2](https://www.msys2.org/) instead, though alternative Windows development environments like [Cygwin](https://www.cygwin.com/) also provide modern compiler toolchains with full OpenMP support.

#### UNIX Build Instructions

Clone the repository and build using these commands:

```bash
git clone --depth=1 https://github.com/gnualmalki/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
cmake --install build  # may require administrator privileges
```

#### Windows (MSYS2) Build Instructions

After installing MSYS2, open the UCRT64 terminal and install the required packages using `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake`, then clone the repository and build:

```bash
git clone --depth=1 https://github.com/gnualmalki/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
cmake --install build  # may require administrator privileges
```

## Usage

```bash
udu <path>... [OPTIONS]...

  -a, --apparent-size            display apparent sizes rather than device usage
  -v, --verbose                  enable verbose output
  -q, --quiet                    enable quiet output (default)
  -X <name>, --exclude=<name>    exclude file or directory
  -h, --help                     display this help and exit
      --version                  display version information and exit
```


## License

This program is distributed under the GNU General Public License Version 3, see the [LICENSE](./LICENSE) file for details.


## Acknowledgements
- Thanks to [OpenMP](https://openmp.org) specification and its implementations by [GCC](https://gcc.gnu.org/) and [LLVM](https://llvm.org/) for making FOSS high-performance parallel computing possible.  
- Thanks to [xxHash](https://github.com/Cyan4973/xxHash) for fast hashing.
