# Benchmarks

Benchmarked with [hyperfine](https://github.com/sharkdp/hyperfine), using this [script](./scripts/benchmark).

## System

ARM64 Arch Linux, 8-core, 8 GB DDR4 RAM machine

## Toolchains

GCC: 15.2.1 (OpenMP: GNU-libgomp 4.5)
Clang: 21.1.5 (OpenMP: LLVM-libomp 5.1)
Zig: 0.15.2

## Build Commands

C (`udu`):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
```

Zig (`udu`):

```bash
zig build -Doptimize=ReleaseFast
```

## Results

Cold cache:

| Command | Mean [s] | Min [s] | Max [s] | Relative |
|:---|---:|---:|---:|---:|
| `GNU du` | 11.831 | 11.831 | 11.831 | 6.06 |
| `Zig udu` | 2.335 | 2.335 | 2.335 | 1.20 |
| `C udu` | 1.951 | 1.951 | 1.951 | 1.00 |

Warm cache:

| Command | Mean [s] | Min [s] | Max [s] | Relative |
|:---|---:|---:|---:|---:|
| `GNU du` | 9.139 | 9.139 | 9.139 | 4.71 |
| `Zig udu` | 2.611 | 2.611 | 2.611 | 1.35 |
| `C udu` | 1.940 | 1.940 | 1.940 | 1.00 |

Note: C binary times were measured with GCC builds. Clang may provide different runtime performance.
