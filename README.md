# UDU

UDU is a fast, multithreaded, cross-platform solution for checking files and directories sizes.

> [!NOTE]
> *C version is %40 faster than the Zig version*


## Dependencies

- **C**: GCC or Clang with OpenMP (v5.0+) support  
- **Zig**: Zig 0.15.1 or later  
- **Unix**: `make` (for installing)  
- **Windows**: C compiler (MinGW/Clang)


## Build from source

```bash
git clone --depth=1 https://github.com/makestatic/udu.git  
cd udu
```

### Unix

**Build C binary**

```bash
make  
sudo make install INSTALL_TARGET=c
```

**Build Zig binary**

```bash
make zig  
sudo make install INSTALL_TARGET=zig
```

### Windows (PowerShell)

**Build C binary**

```powershell
.\Make.ps1 -Target c  
.\Make.ps1 -Install -Target c
```

**Build Zig binary**

```powershell
.\Make.ps1 -Target zig  
.\Make.ps1 -Install -Target zig
```

### Direct Zig build (all platforms)

```bash
cd zig && zig build -Doptimize=ReleaseFast
```

- Unix: `sudo cp ./zig-out/bin/udu /usr/local/bin/udu`  
- Windows: Copy `./zig-out/bin/udu.exe` to a folder in your `PATH`


## Usage

``` bash
udu <path> [-ex=<name_or_path>] [-v|--verbose] [-q|--quiet] [-h|--help] [--version]
```

- `-ex=<name_or_path>` — Exclude file or directory from scanning  
- `-v, --verbose` — Show per-file output  
- `-q, --quiet` — Suppress per-file output  
- `-h, --help` — Show usage  
- `--version` — Show version  


## License

Copyright © 2023–2025 Ali Almalki ([@makestatic](https://github.com/makestatic))  

UDU is distributed under the terms of the [GPLv3](./LICENSE).
