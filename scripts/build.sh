#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"

if [ ! -f "$BUILD/Makefile" ]; then
    echo "==> Build nao configurado. Rodando cmake..."
    mkdir -p "$BUILD"
    cd "$BUILD"
    cmake ..
fi

echo "==> Compilando..."
cd "$BUILD"
make -j$(nproc)

echo ""
echo "Compilado com sucesso: $BUILD/compilador"
