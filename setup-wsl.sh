#!/bin/bash

echo "=========================================="
echo "Setting up Hypnos OS development on WSL"
echo "=========================================="

echo "[1/4] Updating package lists..."
sudo apt-get update -qq

echo "[2/4] Installing build tools (gcc, binutils for 32-bit)..."
sudo apt-get install -y build-essential gcc-multilib g++-multilib

echo "[3/4] Installing GRUB and ISO tools..."
sudo apt-get install -y grub-pc-bin grub-common xorriso mtools

echo "[4/4] Installing QEMU..."
sudo apt-get install -y qemu-system-x86

echo ""
echo "=========================================="
echo "Setup complete!"
echo "=========================================="
echo ""
echo "You can now build and run your OS with:"
echo "  make clean && make run"
echo ""
echo "Or use the convenience script:"
echo "  ./build-and-run.sh"
echo ""
