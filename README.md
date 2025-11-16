# UDU

UDU is an extremely fast and cross-platform tool for measuring file and directory sizes, using a parallel traversal engine implemented with [OpenMP](https://www.openmp.org/) to recursively scan directories, making it significantly faster than traditional tools on multi-core systems.

> **NOTE:** The C implementation can be up to 40% faster than the Zig implementation.

See [Benchmarks](./BENCHMARKS.md) for detailed performance comparisons.

## Quick Install

You can install the Zig implementation using the install script, which provides the easiest cross-platform distribution:

```bash
curl -fsSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash
```

## Build from Source

### C Implementation

The C implementation provides the best performance when built with OpenMP, requiring a modern C compiler such as [GCC](https://gcc.gnu.org/) 9 or later, [Clang](https://clang.llvm.org/) 14 or later, or [MSVC](https://visualstudio.microsoft.com/) 17 or later, along with [CMake](https://cmake.org/) 3.15 or later and [OpenMP](https://www.openmp.org/) 3.1 or later (optional but recommended for parallel processing).

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

The Zig implementation offers easier cross-platform builds while maintaining good performance, requiring only [Zig](https://ziglang.org/) 0.15.2 installed on your system. Clone the repository if you haven't already, then navigate to the zig directory and run the build command:

```bash
cd zig
zig build -Doptimize=ReleaseFast
```

## Usage

```bash
udu <path> [-ex=<name|path>] [-v|--verbose] [-q|--quiet] [-h|--help] [--version]
```

The basic command syntax is `udu <path>` with optional flags. You can use `-ex=<pattern>` to exclude file or directory (can be specified multiple times), `-v` or `--verbose` for detailed output showing individual directory sizes, `-q` or `--quiet` for minimal output showing only the total (default), `-h` or `--help` to display the help message, and `--version` to show the version number.

**Examples:** Run `udu /home` to check your home directory size, use `udu -ex=node_modules .` to analyze the current directory while excluding `node_modules`, or try `udu /var -ex=cache -ex=tmp -v` for multiple exclusions with verbose output to see detailed breakdowns while skipping cache and temporary directories.

## License

This program is distributed under the GNU General Public License version 3, see the [LICENSE](./LICENSE) file for details.
