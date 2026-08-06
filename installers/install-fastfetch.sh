#!/bin/bash
# Install sfetch (minicore fastfetch) into ~/.local/bin and make sure
# ~/.local/bin is on PATH. Run from anywhere:
#   bash ~/minicore/installers/install-fastfetch.sh
set -e

MINICORE="$(cd "$(dirname "$0")/.." && pwd)"
DEST_DIR="$HOME/.local/bin"
DEST="$DEST_DIR/sfetch"

echo "==> building fastfetch..."
make -C "$MINICORE" fastfetch

echo "==> installing $DEST"
mkdir -p "$DEST_DIR"
ln -sf "$MINICORE/fastfetch" "$DEST"

if ! echo "$PATH" | tr ':' '\n' | grep -qx "$DEST_DIR"; then
    if ! grep -q 'HOME/.local/bin' "$HOME/.bashrc" 2>/dev/null; then
        echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
        echo "==> added ~/.local/bin to PATH in ~/.bashrc"
    fi
fi

echo "==> done, run sfetch"
