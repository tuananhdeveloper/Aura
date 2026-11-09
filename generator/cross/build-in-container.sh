#!/usr/bin/env bash
set -euo pipefail

cmake -S generator -B generator/build-cross/linux-x64 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build generator/build-cross/linux-x64 --parallel
ctest --test-dir generator/build-cross/linux-x64 --output-on-failure
cmake --install generator/build-cross/linux-x64 --component generator \
  --prefix generator/dist/linux-x64
(tar -czf generator/dist/aura-generator-linux-x64.tar.gz -C generator/dist/linux-x64 .)

cmake -S generator -B generator/build-cross/windows-x64 \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/generator/cross/windows-x64.cmake"
cmake --build generator/build-cross/windows-x64 --parallel
cmake --install generator/build-cross/windows-x64 --component generator \
  --prefix generator/dist/windows-x64
(cd generator/dist/windows-x64 && python3 -m zipfile -c ../aura-generator-windows-x64.zip .)
