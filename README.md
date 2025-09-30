# udu

A fast, multithreaded, cross-platform alternative to GNU `du`.  While `du` has long been my tool of choice for checking file and directory sizes, it has become slow, inconsistent, and occasionally inaccurate. `udu` isn’t a full drop-in replacement yet, but it gets the job done... for me at least.

## Installation

```console
# Install via script (recommended)
curl -fsSL https://raw.githubusercontent.com/makestatic/udu/main/scripts/install.sh | bash

# Build from source (Zig 0.15.1 only)
git clone --depth=1 https://github.com/makestatic/udu.git
cd udu

# if make is available
make
sudo make install

# if make is not available
zig build -Doptimize=ReleaseFast
sudo cp ./zig-out/bin/udu /usr/bin/
```

## License
<sub>
Copyright © 2023–2025 <a href="https://github.com/makestatic">Ali Almalki</a>.  
`udu` is distributed under the terms of the [GPLv3 license](LICENSE).
</sub>
