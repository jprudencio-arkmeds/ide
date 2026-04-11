#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "==> Instalando dependencias..."
sudo apt-get install -y qtbase5-dev cmake build-essential

echo "==> Configurando build..."
mkdir -p "$ROOT/build"
cd "$ROOT/build"
cmake ..

echo ""
echo "Setup concluido. Execute ./build.sh para compilar."
