#!/bin/bash
set -e

echo "=== Cleaning previous build ==="
rm -rf build
mkdir build
cd build

echo "=== Configuring CMake (Release) ==="
cmake -DCMAKE_BUILD_TYPE=Release ..

if command -v nproc >/dev/null 2>&1; then
    JOBS=$(nproc)
else
    JOBS=$(sysctl -n hw.ncpu)
fi

echo "=== Building ==="
cmake --build . -j"${JOBS}"

echo "=== Build complete ==="
echo "Terminal 1: ./build/consumer"
echo "Terminal 2: ./build/producer 1048576"
