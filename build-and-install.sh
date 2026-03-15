#!/bin/bash
set -e

echo "============================================"
echo " ShmeaDB - Build and Install (Linux)"
echo "============================================"
echo

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Verify prerequisites
command -v cmake >/dev/null 2>&1 || { echo "[ERROR] cmake not found."; exit 1; }
command -v make >/dev/null 2>&1 || { echo "[ERROR] make not found."; exit 1; }
echo "[OK] Prerequisites found."

# Configure
mkdir -p build && cd build
echo "[STEP] Configuring..."
cmake ..
echo "[OK] Configure succeeded."
echo

# Build
echo "[STEP] Building..."
make -j"$(nproc)"
echo "[OK] Build succeeded."
echo

# Install
echo "[STEP] Installing to ~/.local ..."
make install
echo "[OK] Installed to ~/.local"
echo

echo "============================================"
echo " Done! ShmeaDB installed to ~/.local"
echo "============================================"
