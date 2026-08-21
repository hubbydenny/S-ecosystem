#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
PREFIX="${PREFIX:-/usr/local}"
BIN_DIR="$PREFIX/bin"

if [ "$(id -u)" -ne 0 ]; then
    echo "[*] Need root. Enter sudo password:"
    exec sudo "$0" "$@"
fi

echo "========================================="
echo "  S-ecosystem Installer"
echo "========================================="
echo ""

cd "$ROOT"

echo "[*] Building all..."
make clean > /dev/null 2>&1 || true
make -j$(nproc)

echo "[*] Installing to $BIN_DIR..."
mkdir -p "$BIN_DIR"

Installed=()

for bin in sfetch scat sls; do
    if [ -f "$ROOT/$bin" ] && [ -x "$ROOT/$bin" ]; then
        install -Dm755 "$ROOT/$bin" "$BIN_DIR/$bin"
        Installed+=("$bin")
        echo "  - $bin -> $BIN_DIR/$bin"
    fi
done

echo ""
echo "========================================="
echo "  Installed: ${Installed[*]}"
echo "========================================="
echo ""
for bin in "${Installed[@]}"; do
    echo "  Run: $bin"
done
