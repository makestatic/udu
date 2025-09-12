# udu
A Fast, Multithreaded, Cross-Platform alternative to GNU du.

## Installation
```console
curl -sSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash
```

## Build

Requirements:
- [Unix OS](https://en.wikipedia.org/wiki/Unix)
- [Zig 0.15.1+](https://ziglang.org/download/)
- [GNU Make](https://www.gnu.org/software/make/)

```console
git clone https://github.com/makestatic/udu.git
cd udu
make
sudo make install
```

## Usage

```console
udu <path> [OPTIONS]

Options:
    -ex=<path>      Exclude directory (repeatable)
    -v, --verbose   Verbose output
    -q, --quiet     Quiet output (default)
    -h, --help      Show this help message
    --version       Show version

EXAMPLE:
    udu ~/ -v
```

## License
[GPLv3 or later](LICENSE)
