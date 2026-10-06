#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
component="$project_dir/build-macos/BeatFlip_artefacts/Release/AU/Dustbox.component"
destination="$HOME/Library/Audio/Plug-Ins/Components/Dustbox.component"
[[ "$(uname -s)" == "Darwin" ]] || { echo "Audio Units require macOS." >&2; exit 1; }
[[ -d "$component" ]] || { echo "Run bash scripts/build-macos.sh first." >&2; exit 1; }
legacy="$HOME/Library/Audio/Plug-Ins/Components/Beat Flip.component"
if [[ -e "$legacy" ]]; then
  echo "Move the previous Beat Flip.component out of the Components folder before installing Dustbox; both use the same AU identity." >&2
  exit 1
fi
if [[ -e "$destination" ]]; then
  echo "Already installed at $destination. Move that version aside before installing this one." >&2
  exit 1
fi
mkdir -p "$(dirname "$destination")"
ditto "$component" "$destination"
echo "Installed. Quit and reopen Logic, then validate with: auval -v aufx BtFp Rbjm"
