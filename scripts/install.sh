#!/usr/bin/env bash
set -euo pipefail

GREEN="\033[0;32m"
YELLOW="\033[1;33m"
RED="\033[0;31m"
BLUE="\033[0;34m"
NC="\033[0m"

OWNER="makestatic"
REPO="udu"
BIN_NAME="$REPO"

printf "%b\n" "${BLUE}Detecting OS and architecture...${NC}"

OS="$(uname | tr '[:upper:]' '[:lower:]')"
ARCH="$(uname -m)"

case "$ARCH" in
    x86_64) ARCH="x86_64" ;;
    aarch64|arm64) ARCH="aarch64" ;;
    *) printf "%b\n" "${RED}[!] Unsupported architecture: $ARCH${NC}"; exit 1 ;;
esac

case "$OS" in
    darwin) OS="apple-darwin" ;;
    mingw*|cygwin*|msys*) OS="windows" ;;
    freebsd) OS="freebsd" ;;
    linux)
        if command -v ldd >/dev/null 2>&1 && ldd --version 2>&1 | grep -q musl; then
            OS="linux-musl"
        else
            OS="linux-gnu"
        fi
        ;;
    *) printf "%b\n" "${RED}[!] Unsupported OS: $OS${NC}"; exit 1 ;;
esac

printf "%b\n" "${GREEN}Detected OS: $OS, ARCH: $ARCH${NC}"

printf "%b\n" "${BLUE}Fetching latest release tag from GitHub...${NC}"
TAG=$(curl -fsSL "https://api.github.com/repos/$OWNER/$REPO/releases/latest" \
      | grep '"tag_name":' | sed -E 's/.*"([^"]+)".*/\1/')

if [ -z "$TAG" ]; then
    printf "%b\n" "${RED}[!] Failed to fetch release tag${NC}"
    exit 1
fi

FILE_NAME="${BIN_NAME}-${ARCH}-${OS}.tar.gz"
DOWNLOAD_URL="https://github.com/$OWNER/$REPO/releases/download/$TAG/$FILE_NAME"

printf "%b\n" "${BLUE}Downloading $FILE_NAME...${NC}"
TMP_DIR=$(mktemp -d)
curl -fsSL "$DOWNLOAD_URL" -o "$TMP_DIR/$FILE_NAME"

printf "%b\n" "${BLUE}Extracting...${NC}"
tar -xzf "$TMP_DIR/$FILE_NAME" -C "$TMP_DIR"

if [ "$OS" = "pc-windows-msvc" ]; then
    BIN_NAME="${BIN_NAME}.exe"
fi

BIN=$(find "$TMP_DIR" -name "$BIN_NAME" -type f | head -1 || true)

if [ ! -f "$BIN" ]; then
    printf "%b\n" "${RED}[!] Could not find $BIN_NAME in extracted archive${NC}"
    printf "%b\n" "${YELLOW}Contents of extracted archive:${NC}"
    find "$TMP_DIR" -type f
    rm -rf "$TMP_DIR"
    exit 1
fi

INSTALL_DIR="/usr/local/bin"
if [ ! -w "$INSTALL_DIR" ]; then
    printf "%b\n" "${YELLOW}[!] No write access to $INSTALL_DIR, using ~/.local/bin${NC}"
    INSTALL_DIR="$HOME/.local/bin"
    mkdir -p "$INSTALL_DIR"
fi

printf "%b\n" "${BLUE}Installing $BIN_NAME to $INSTALL_DIR...${NC}"
cp "$BIN" "$INSTALL_DIR/$BIN_NAME"
chmod +x "$INSTALL_DIR/$BIN_NAME"

printf "%b\n" "${GREEN}$BIN_NAME installed successfully!${NC}"
printf "%b\n" "${BLUE}Installed version: ${TAG}${NC}"

rm -rf "$TMP_DIR"
