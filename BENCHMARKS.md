# Benchmarks

Benchmarked with [hyperfine](https://github.com/sharkdp/hyperfine), using this [script](./scripts/benchmark).

## System

ARM64 Arch Linux, 8-core, 8 GB DDR4 RAM machine

## Toolchains

- GCC: 15.2.1 (OpenMP: GNU-libgomp 4.5)
- Clang: 21.1.5 (OpenMP: LLVM-libomp 5.1)
- Zig: 0.15.2

## Build Commands

- C
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=ON -DENABLE_LTO=ON
cmake --build build
```

- Zig

```bash
zig build -Doptimize=ReleaseFast
```

## Results

| Program | Mean [s] | Min [s] | Max [s] | Relative |
|:---|---:|---:|---:|---:|
| `GNU du` | 10.008 ± 1.339 | 9.368 | 12.403 | 5.49 ± 0.78 |
| `Zig udu` | 2.302 ± 0.132 | 2.110 | 2.460 | 1.26 ± 0.09 |
| `C udu` | 1.824 ± 0.086 | 1.729 | 1.928 | 1.00 |
