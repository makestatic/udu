param(
    [ValidateSet("c","zig","clean","install")]
    [string]$Target = "c"
)

$BUILD_DIR = "build"
$BIN_DIR   = "$BUILD_DIR\bin"
$TARGET    = "udu"
$C_SRC     = "c\udu.c"

function Build-C {
    Write-Host "Building C binary..."
    if (-Not (Test-Path $BIN_DIR)) { New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null }
    cc -O3 -std=gnu99 -Wall -Wextra -fopenmp -march=native -flto -o "$BIN_DIR\$TARGET.exe" $C_SRC
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Build-Zig {
    Write-Host "Building Zig binary..."
    Push-Location zig
    zig build -Doptimize=ReleaseFast --prefix "../$BUILD_DIR" --summary all -Dstrip
    $ret = $LASTEXITCODE
    Pop-Location
    if ($ret -ne 0) { exit $ret }
}

function Clean {
    Write-Host "Cleaning build directories..."
    if (Test-Path $BUILD_DIR) { Remove-Item -Recurse -Force $BUILD_DIR }
    New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null
}

function Install {
    Write-Host "Installing binary..."
    if (Test-Path "$BIN_DIR\$TARGET.exe") {
        $dest = "C:\Windows\System32\$TARGET.exe"
        Copy-Item "$BIN_DIR\$TARGET.exe" -Destination $dest -Force
        Write-Host "Installed binary to $dest"
    } else {
        Write-Host "No binary found to install."
    }
}

switch ($Target) {
    "c"       { Build-C }
    "zig"     { Build-Zig }
    "clean"   { Clean }
    "install" { Install }
}
