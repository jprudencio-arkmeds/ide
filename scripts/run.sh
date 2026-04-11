#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"

echo "==> Compilando..."
cd "$BUILD"
make -j$(nproc) 2>&1 | grep -v "^make\[" || true

echo "==> Executando..."
"$BUILD/compilador"
