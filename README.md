# udu
A fast, multithreaded, cross-Platform alternative to GNU du.

## Installation
```console
curl -sSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash
```

## Build from source

Requirements:
- [Zig 0.15.1+](https://ziglang.org/download/)
- [GNU Make](https://www.gnu.org/software/make/) (optional)

```console
git clone --depth=1 https://github.com/makestatic/udu.git
cd udu

# if Make avalibale
make
sudo make install

# if Make is not avalibale
zig build -Doptimize=$(OPT_SAFE) --prefix $(BUILD_DIR) --summary all -Dstrip
```

## Usage

```console
udu <path> [OPTIONS]

Options:
    -ex=<path>      Exclude directory (repeatable)
    -q, --quiet     Quiet output (default)
    -v, --verbose   Verbose output
    -h, --help      Show this help message
    --version       Show version

EXAMPLE:
    udu ~/ -v
```

## License
[GPLv3 or later](LICENSE)
