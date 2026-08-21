#!/bin/sh
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PREFIX="${PREFIX:-/usr/local}"
BIN_DIR="$PREFIX/bin"
OS="$(uname -s)"

if [ "$1" = "uninstall" ]; then
    if [ "$(id -u)" -ne 0 ]; then
        exec ${DOAS:-sudo} sh "$0" uninstall
    fi
    for bin in sfetch scat sls; do
        if [ -f "$BIN_DIR/$bin" ]; then
            rm -f "$BIN_DIR/$bin"
            echo "[-] Removed $BIN_DIR/$bin"
        fi
    done
    exit 0
fi

if [ "$(id -u)" -ne 0 ]; then
    if command -v doas >/dev/null 2>&1; then
        exec doas sh "$0" "$@"
    else
        exec sudo sh "$0" "$@"
    fi
fi

echo "  S-ecosystem Installer ($OS)"
echo ""

cd "$ROOT"

CXX="${CXX:-}"
if [ -z "$CXX" ]; then
    for c in c++ clang++ g++ eg++ ; do
        if command -v "$c" >/dev/null 2>&1; then
            CXX="$c"
            break
        fi
    done
fi
if [ -z "$CXX" ]; then
    echo "[!] No C++ compiler found."
    echo "    FreeBSD:  pkg install llvm"
    echo "    OpenBSD:  pkg_add llvm"
    echo "    NetBSD:   pkgin install gcc12"
    exit 1
fi
echo "[*] Compiler: $CXX"

cat > /tmp/secos-conftest.cpp <<'EOF'
int main() { return 0; }
EOF
CXXSTD=""
for std in c++20 c++2a gnu++20; do
    if "$CXX" "-std=$std" -o /tmp/secos-conftest /tmp/secos-conftest.cpp 2>/dev/null; then
        CXXSTD="-std=$std"
        break
    fi
done
rm -f /tmp/secos-conftest /tmp/secos-conftest.cpp
if [ -z "$CXXSTD" ]; then
    echo "[!] $CXX does not support C++20."
    echo "    FreeBSD:  pkg install llvm       (use CXX=clang++)"
    echo "    OpenBSD:  pkg_add llvm           (use CXX=c++)"
    echo "    NetBSD:   pkgin install gcc12 && CXX=/usr/pkg/gcc12/bin/g++ sh installers/install-bsd.sh"
    exit 1
fi
echo "[*] Standard: $CXXSTD"

echo "[*] Building..."
FLAGS="$CXXSTD -Wall -Wextra -O2 -Isrc/helpers -Isrc/sfetch -Iexternal/tomlplusplus/include"

"$CXX" $FLAGS src/sfetch/main.cpp -o sfetch
"$CXX" $FLAGS src/cat/scat.cpp     -o scat
"$CXX" $FLAGS src/simple/ls.cpp    -o sls

echo "[*] Installing to $BIN_DIR..."
mkdir -p "$BIN_DIR"

Installed=""
for bin in sfetch scat sls; do
    if [ -f "$ROOT/$bin" ] && [ -x "$ROOT/$bin" ]; then
        install -m755 "$ROOT/$bin" "$BIN_DIR/$bin"
        Installed="$Installed $bin"
        echo "  - $bin -> $BIN_DIR/$bin"
    fi
done

rm -f "$ROOT/sfetch" "$ROOT/scat" "$ROOT/sls"

echo ""
echo "  Installed:$Installed"
echo ""
for bin in $Installed; do
    echo "  Run: $bin"
done
