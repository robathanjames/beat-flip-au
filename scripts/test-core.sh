#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
mkdir -p "$project_dir/build-core"
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic \
  -I"$project_dir/Source" "$project_dir/Source/GlitchEngine.cpp" \
  "$project_dir/Tests/EngineTests.cpp" "$project_dir/Tests/AllocationGuard.cpp" \
  -o "$project_dir/build-core/beatflip_tests"
"$project_dir/build-core/beatflip_tests"
