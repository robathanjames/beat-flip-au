#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
[[ "$(uname -s)" == "Darwin" ]] || { echo "Audio Units require macOS." >&2; exit 1; }
variant="${1:-both}"
components=()
case "$variant" in
  effect) components+=("BeatFlip_artefacts/Release/AU/Dustbox.component") ;;
  synth) components+=("DustboxSynth_artefacts/Release/AU/Dustbox Synth.component") ;;
  both) components+=("BeatFlip_artefacts/Release/AU/Dustbox.component" "DustboxSynth_artefacts/Release/AU/Dustbox Synth.component") ;;
  *) echo "Choose both, effect or synth." >&2; exit 1 ;;
esac
folder="$HOME/Library/Audio/Plug-Ins/Components"
if [[ "$variant" != synth && -e "$folder/Beat Flip.component" ]]; then
  echo "Move the old Beat Flip.component aside before installing Dustbox; they share an AU identity." >&2; exit 1
fi
# Check every target before copying, so a previous installation is never overwritten.
for component in "${components[@]}"; do
  [[ -d "$project_dir/build-macos/$component" ]] || { echo "Build the requested variant first." >&2; exit 1; }
  [[ ! -e "$folder/${component##*/}" ]] || { echo "Move the existing ${component##*/} aside before installing." >&2; exit 1; }
done
mkdir -p "$folder"
for component in "${components[@]}"; do
  ditto "$project_dir/build-macos/$component" "$folder/${component##*/}"
done
echo "Installed. Restart Logic. Validate: auval -v aufx BtFp Rbjm (effect), auval -v aumu DbSy Rbjm (synth)."
