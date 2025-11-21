# UDU
**UDU** is an extremely fast, cross-platform tool for summarizing file and directory sizes by recursively scanning directories using a parallel traversal engine implemented with [OpenMP](https://www.openmp.org/), making it significantly faster than traditional tools on multi-core systems.

[![CI](https://github.com/gnualmalki/udu/actions/workflows/ci.yml/badge.svg)](https://github.com/gnualmalki/udu/actions/workflows/ci.yml)

See [Benchmarks](./BENCHMARKS.md).

## Build Instructions
Building **UDU** requires a modern C compiler such as [GCC](https://gcc.gnu.org/) 9 or later, [Clang](https://clang.llvm.org/) 14 or later, or [MSVC](https://visualstudio.microsoft.com/) 17.2 or later, along with [CMake](https://cmake.org/) 3.15 or later and [OpenMP](https://www.openmp.org/) 3.0 or later (optional: parallel processing).

**NOTE ON WINDOWS:** MSVC versions less than 17.2 lack support for the OpenMP 3.0 features required by this program. If you're using an older MSVC version, the build system will automatically detect this and compile without OpenMP (single-threaded mode). For maximum performance on Windows, we recommend using [MSVC](https://visualstudio.microsoft.com/) 17.2 (VS 2022) or later with LLVM OpenMP runtime (the build system handles that for you), or alternatively GCC or Clang through [MSYS2](https://www.msys2.org/) or [Cygwin](https://www.cygwin.com/), which provide modern toolchains with complete OpenMP 3.0 support.

#### UNIX
Clone the repository and build using these commands:

```bash
git clone --depth=1 https://github.com/gnualmalki/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
```

#### Windows (MSYS2)
After installing MSYS2, open the UCRT64 terminal and install the required packages using `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake`, then follow the UNIX instructions above.

#### Windows (MSVC)
Using Developer Command Prompt or PowerShell with Visual Studio environment:

```cmd
git clone --depth=1 https://github.com/gnualmalki/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build --config Release
```

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

Report bugs to <https://github.com/gnualmalki/udu/issues>
```

## License
THIS PROGRAM IS DISTRIBUTED UNDER GPL-3-OR-LATER, SEE THE [LICENSE](./LICENSE) FILE FOR DETAILS.

## Acknowledgements
Thanks to [OpenMP ARB](https://openmp.org) for OpenMP specifications and Thanks to its implementations by [GCC](https://gcc.gnu.org/) and [LLVM](https://llvm.org/) for making FOSS high-performance parallel computing possible.
