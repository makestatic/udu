# UDU
UDU is a fast, multithreaded, cross-platform solution for checking files and directories sizes.



> [!NOTE]
> *The C version is almost 40% faster than the Zig version at some cases*

[Here's a Reddit post I made discussing this outcome](https://www.reddit.com/r/C_Programming/comments/1oujlds/ported_my_zig_tool_to_c_and_got_almost_a_40/).


## Quick Install (zig version only)

```bash
curl -fsSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash
```

## Build from Source

### Minimal requirements

- GCC (10.5) / Clang (14) / MSVC (not tested)
- OpenMP 3.1 (optional but recommended)
- CMake 3.15
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

**Options**
- `-ex=<n>` — exclude file/directory
- `-v, --verbose` — verbose output
- `-q, --quiet` — quiet output
- `-h, --help` — show help message
- `--version` — show version

**Examples**
```bash
udu /home                      # Scan home directory
udu -ex=node_modules .         # Exclude node_modules
udu /var -ex=cache -ex=tmp -v  # Multiple exclusions, verbose
```

## License

GPLv3
