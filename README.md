# UDU

UDU is a **fast, multithreaded, cross-platform tool** for checking files and directories sizes. 

> [!NOTE]
> *The C version can be up to **40% faster** than the Zig version in some cases*

See: [Benchmarks](./BENCHMARK) | [Reddit post I made discussing this outcome](https://www.reddit.com/r/C_Programming/comments/1oujlds/ported_my_zig_tool_to_c_and_got_almost_a_40/)

## Quick Install (Zig version)
```bash
curl -fsSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash
```

## Build from Source

### Requirements

- GCC ≥ 9 / Clang ≥ 11 / MSVC ≥ 16
- OpenMP ≥ 3 (optional but recommended)
> [!WARNING]
> *MSVC lacks support for OpenMP 3.0*
- CMake ≥ 3.15

If you want to build the Zig version, you need:
- Zig 0.15.2


Clone the repository:
```bash
git clone --depth=1 https://github.com/makestatic/udu.git
cd udu
```

### C Version

Build & install:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
cmake --install build  # may require admin
```

Optional build flags:
```bash
cmake -B build -DENABLE_OPENMP=OFF  # Disable parallel processing
cmake -B build -DENABLE_LTO=OFF     # Disable link-time optimization
```

### Zig Version
```bash
cd zig
zig build -Doptimize=ReleaseSafe
```

## Usage
```bash

udu <path> [-ex=<name|path>] [-v|--verbose] [-q|--quiet] [-h|--help] [--version]

    OPTIONS:
    -ex=<name>       Exclude file or directory
    -v, --verbose    Verbose output
    -q, --quiet      Quiet output (default)
    -h, --help       Show help message
    --version        Show version

    EXAMPLES:
    udu /home
    udu -ex=node_modules .
    udu /var -ex=cache -ex=tmp -v

```

## License

This program is distributed under the terms of the GNU General Public License version 3 (GPL-3.0); See [LICENSE](./LICENSE) file.
