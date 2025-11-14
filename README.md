# UDU
UDU is a fast, multithreaded, cross-platform tool for checking files and directories sizes. The C implementation can be up to 40% faster than the Zig implementation in some cases.

See [Benchmarks](./BENCHMARK) and [Reddit discussion](https://www.reddit.com/r/C_Programming/comments/1oujlds/ported_my_zig_tool_to_c_and_got_almost_a_40/) about the performance outcome.


## Quick Install
You can install the Zig implementation using the install script. The Zig implementation is provided as the default install option for easier cross-platform distribution.

```bash
curl -fsSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash
```

## Build from Source

### C Implementation
The C implementation provides the best performance, especially when built with [OpenMP](https://www.openmp.org/), to build it you need a modern C compiler such as [GCC](https://gcc.gnu.org/) 9 or later, [Clang](https://clang.llvm.org/) 14 or later, or [MSVC](https://visualstudio.microsoft.com/) 17 or later. You will also need [CMake](https://cmake.org/) 3.15. [OpenMP](https://www.openmp.org/) 3.1 or later (optional but recommended). Note that MSVC only supports OpenMP 2.0 so no parallel processing when built with MSVC.

(UNIX) Clone the repository and build:
```bash
git clone --depth=1 https://github.com/makestatic/udu.git
cd udu
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
cmake --install build  # may require administrator privileges
```

### Zig Implementation
The Zig implementation offers easier cross-platform builds and good performance. to build it you only need [Zig](https://ziglang.org/) 0.15.2.

Clone the repository if you haven't already, then navigate to the zig directory and run the following command:

```bash
zig build -Doptimize=ReleaseSafe
```

## Usage
The basic command syntax is `udu <path>` with optional flags. Use `-ex=<name>` to exclude files or directories, `-v` or `--verbose` for detailed output, `-q` or `--quiet` for minimal output (default), `-h` or `--help` to display help, and `--version` to show the version number.

Examples: Run `udu /home` to check your home directory size. Use `udu -ex=node_modules .` to check the current directory while excluding `node_modules`. For multiple exclusions with verbose output, try `udu /var -ex=cache -ex=tmp -v`.


## License
This program is distributed under the [GNU General Public License version 3](https://www.gnu.org/licenses/gpl-3.0.html). See the [LICENSE](./LICENSE) file for full details.
