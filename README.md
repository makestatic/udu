# UDU
UDU is a fast, multithreaded, cross-platform solution for checking files and directories sizes.



> [!NOTE]
> *The C version is almost 40% faster than the Zig version at some cases*

[Benchmarks](./BENCHMARK).

[Here's a Reddit post I made discussing this outcome](https://www.reddit.com/r/C_Programming/comments/1oujlds/ported_my_zig_tool_to_c_and_got_almost_a_40/).


## Quick Install (zig version only)

```bash
curl -fsSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash
```

## Build from Source

### Minimal requirements

- GCC (8.1+) / Clang (11.1+) / MSVC (16+)
- OpenMP 3.1+ (optional but recommended)
- CMake 3.15

If you want to build the zig version you need:
- Zig 0.15.2

```bash
git clone --depth=1 https://github.com/makestatic/udu.git
cd udu
```

### Build C version & Install

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
cmake --install build  # admin required
```

**Build options**
```bash
cmake -B build -DENABLE_OPENMP=OFF  # Disable parallel processing
cmake -B build -DENABLE_LTO=OFF     # Disable link-time optimization
```


### Build Zig version
Navigate to `zig` directory and run the following command:

```bash
zig build -Doptimize=ReleaseSafe
```


## Usage

```bash
udu <path> 
    [-ex=<name|path>]
    [-v|--verbose]
    [-q|--quiet]
    [-h|--help]
    [--version]
```

**OPTIONS:**
- `-ex=<name>`      Exclude file or directory
- `-v, --verbose`   Verbose output
- `-q, --quiet`     Quiet output (default)
- `-h, --help`      Show help message
- `--version`       Show version

**EXAMPLES:**
```bash
udu /home                      # Scan home directory
udu -ex=node_modules .         # Exclude node_modules
udu /var -ex=cache -ex=tmp -v  # Multiple exclusions, verbose
```

## License

[GPLv3](./LICENSE)
