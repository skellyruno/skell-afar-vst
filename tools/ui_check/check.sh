#!/usr/bin/env bash
# Developer check for the editor/processor sources without JUCE: compile them against the structural mock
# (-Wall -Wextra -Wshadow), render the real editor off-screen, and report text that overflows its box.
# Usage: tools/ui_check/check.sh [outdir]      Needs: g++ (C++17), python3 + Pillow + numpy.
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${1:-/tmp/skell_ui_check}"; mkdir -p "$out"
inc=(-Itools/ui_check -Itests/stub -ISource)
for f in Source/PluginProcessor.cpp Source/PluginEditor.cpp; do
  echo "compile-check $f"; g++ -std=c++17 -fsyntax-only -Wall -Wextra -Wshadow "${inc[@]}" "$f"
done
g++ -std=c++17 -O1 -Wall -Wextra -DUI_ASSETS_DIR='"Assets"' "${inc[@]}" tools/ui_check/preview.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp -o "$out/preview"
"$out/preview" effort=.55 distance=12 angle=40 space=5 meterPeak=.6 meterDistance=12 meterClarity=.7 meterDrr=-6 > "$out/main.txt"
python3 tools/ui_check/render.py "$out/main.txt" "$out/main.png" 1100 700 Assets
"$out/preview" detail > "$out/detail.txt"
python3 tools/ui_check/render.py "$out/detail.txt" "$out/detail.png" 774 489 Assets
echo "OK: images in $out (main.png, detail.png); LAYOUT WARNING lines above, if any, need attention"
