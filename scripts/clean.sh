#!/bin/bash

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"

echo "==> Limpando build..."
rm -rf "$BUILD"

echo "Build removido. Execute ./setup.sh para reconfigurar."
