#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="aura-generator-cross:ubuntu-24.04"

docker build --platform linux/amd64 -t "$image" -f "$root/generator/cross/Dockerfile" "$root"
docker run --rm --platform linux/amd64 -v "$root:/src" -w /src "$image" \
  bash /src/generator/cross/build-in-container.sh

printf 'Cross-build packages created in %s/generator/dist\n' "$root"
