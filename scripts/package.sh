#!/usr/bin/env bash
# Package a built Red/Blue variant into a standalone, runnable zip.
#
# Usage: scripts/package.sh <red|blue> <stock|extended> [version]
#
# Output: dist/Pokemon_<Game>_<Variant>_<version>.zip  (dist/ is gitignored).
#
# Bifurcation (mirrors PokemonYellowDecomp): ONE stock exe (Pokemon_Red_Blue)
# runs both stock Red and Blue; ONE extended exe (Pokemon_Red_Extended) runs both
# Gen2-grafted ROMs. Red and Blue share identical code layout, so the variant exe
# is the same for both games — the bundled ROM + rom.cfg select which game runs.
# Four packages: {red,blue} x {stock,extended}.
#
# PRIVATE artifact: the zip bundles a ROM (stock ROMs are copyrighted; the
# extended ROMs are ROM-derivative). Do NOT distribute — local/personal use only,
# which is why dist/ is never committed.
set -euo pipefail
G="${1:?usage: scripts/package.sh <red|blue> <stock|extended> [version]}"
V="${2:?usage: scripts/package.sh <red|blue> <stock|extended> [version]}"
VER="${3:-v0.0.4}"

P="$(cd "$(dirname "$0")/.." && pwd)"
MGW=/c/msys64/mingw64/bin

case "$V" in
  stock)    EXE="$P/generated/build/Pokemon_Red_Blue.exe";      EXENAME=Pokemon_Red_Blue.exe;;
  extended) EXE="$P/generated_ext/build/Pokemon_Red_Extended.exe"; EXENAME=Pokemon_Red_Extended.exe;;
  *) echo "variant must be stock|extended"; exit 1;;
esac

case "$G/$V" in
  red/stock)      ROM="$P/roms/Pokemon Red (UE) [S][!].gb";  GCAP=Red;;
  blue/stock)     ROM="$P/roms/Pokemon Blue (UE) [S][!].gb"; GCAP=Blue;;
  red/extended)   ROM="$P/roms/Pokemon_Red_Extended.gb";     GCAP=Red;;
  blue/extended)  ROM="$P/roms/Pokemon_Blue_Extended.gb";    GCAP=Blue;;
  *) echo "game must be red|blue"; exit 1;;
esac
VCAP="$(tr '[:lower:]' '[:upper:]' <<<"${V:0:1}")${V:1}"

[ -f "$EXE" ] || { echo "build it first ($EXE missing)"; exit 1; }
[ -f "$ROM" ] || { echo "ROM missing: $ROM"; exit 1; }

STAGE="$P/dist/Pokemon_${GCAP}_${VCAP}_$VER"
ZIP="$P/dist/Pokemon_${GCAP}_${VCAP}_$VER.zip"
rm -rf "$STAGE" "$ZIP"; mkdir -p "$STAGE"

cp "$EXE" "$STAGE/$EXENAME"
cp "$ROM" "$STAGE/rom.gb"
printf 'rom.gb\n' > "$STAGE/rom.cfg"   # relative -> resolves next to the exe

# Auto-derive the DLL set: walk the exe's import table transitively, bundling
# only DLLs that live in the mingw64 toolchain dir (system DLLs ship with
# Windows). Seed libEGL (SDL dlopen's the ANGLE driver at runtime, so it isn't in
# the static import table); the walk then pulls its own deps.
declare -A seen
queue=("$EXE")
for r in libEGL; do
  if [ -f "$MGW/$r.dll" ] && [ -z "${seen[$r.dll]:-}" ]; then
    seen[$r.dll]=1; cp "$MGW/$r.dll" "$STAGE/"; queue+=("$MGW/$r.dll")
  fi
done
while [ ${#queue[@]} -gt 0 ]; do
  cur="${queue[0]}"; queue=("${queue[@]:1}")
  for d in $(objdump.exe -p "$cur" 2>/dev/null | sed -n 's/.*DLL Name:[[:space:]]*//p' | tr -d '\r'); do
    if [ -f "$MGW/$d" ] && [ -z "${seen[$d]:-}" ]; then
      seen[$d]=1; cp "$MGW/$d" "$STAGE/"; queue+=("$MGW/$d")
    fi
  done
done
echo "  bundled ${#seen[@]} runtime DLLs"

# tiny run.bat so the cfg/saves resolve next to the exe regardless of launch method
cat > "$STAGE/run.bat" <<BAT
@echo off
cd /d "%~dp0"
start "" "$EXENAME" %*
BAT

PS="/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
"$PS" -NoProfile -Command \
  "Compress-Archive -Path '$(cygpath -w "$STAGE")\\*' -DestinationPath '$(cygpath -w "$ZIP")' -Force"

echo "PACKAGED: dist/Pokemon_${GCAP}_${VCAP}_$VER.zip  ($(du -h "$ZIP" | cut -f1)), $(ls "$STAGE" | wc -l) files"
echo "  contents: $EXENAME + ${#seen[@]} DLLs + rom.gb + rom.cfg + run.bat"
