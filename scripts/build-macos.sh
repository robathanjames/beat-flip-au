#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "Build the AU on a Mac. For portable engine tests, use scripts/test-core.sh." >&2
  exit 1
fi
command -v cmake >/dev/null || { echo "Install CMake first: https://cmake.org/download/" >&2; exit 1; }
cmake -S "$project_dir" -B "$project_dir/build-macos" -G 'Unix Makefiles' \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build "$project_dir/build-macos" --config Release --parallel 4
ctest --test-dir "$project_dir/build-macos" -C Release --output-on-failure
echo "AU: $project_dir/build-macos/BeatFlip_artefacts/Release/AU/Beat Flip.component"
