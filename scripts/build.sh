#!/usr/bin/env bash
# Build a Red/Blue variant end-to-end, then package both games for it.
#
# Usage: scripts/build.sh <stock|extended> [version]
#
# One exe per variant runs BOTH games (Red & Blue share code layout):
#   stock    -> generated/build/Pokemon_Red_Blue.exe       (stock Red + Blue)
#   extended -> generated_ext/build/Pokemon_Red_Extended.exe (Gen2 Red + Blue)
#
# Encodes the exact steps verified by hand; the extended path injects Gen2 from
# the sibling PokemonYellowDecomp/inject_red.py, builds both ROMs, refreshes the
# CRCs in extras.c, recompiles, and (re)links. Then packages all relevant zips.
set -euo pipefail
V="${1:?usage: scripts/build.sh <stock|extended> [version]}"
VER="${2:-v0.0.4}"
P="$(cd "$(dirname "$0")/.." && pwd)"
WS="$(cd "$P/.." && pwd)"                       # workspace root
POKERED="$WS/pokered"
DECOMP="$WS/PokemonYellowDecomp"
RGBDS="$DECOMP/tools/rgbds/"
GBRECOMP="$WS/gb-recompiled/build/bin/gbrecomp.exe"
MGW=/c/msys64/mingw64/bin
export PATH="$MGW:/c/msys64/usr/bin:/c/Windows/system32:/c/Windows"
export TMPDIR="C:/msys64/tmp" TMP="C:/msys64/tmp" TEMP="C:/msys64/tmp"

# kill any running instance so ld/ninja can replace the exe
"/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe" -NoProfile -Command \
  "Get-Process Pokemon_Red_Blue,Pokemon_Red_Extended -ErrorAction SilentlyContinue | Stop-Process -Force" 2>/dev/null || true

case "$V" in
  stock)
    echo "== [stock] static recompile Red+Blue =="
    ( cd "$P" && "$GBRECOMP" --config pokemon_red_blue.toml )
    "$MGW/cmake.exe" -G Ninja -S "$P/generated" -B "$P/generated/build"
    "$MGW/ninja.exe" -C "$P/generated/build"
    echo "BUILT: generated/build/Pokemon_Red_Blue.exe"
    "$P/scripts/package.sh" red  stock "$VER"
    "$P/scripts/package.sh" blue stock "$VER"
    ;;
  extended)
    echo "== [extended] reset pokered to pristine =="
    git -C "$POKERED" checkout -- . ; git -C "$POKERED" clean -fdq
    echo "== [extended] inject Gen2 (MOVE_MODE=simple) =="
    ( cd "$DECOMP" && MOVE_MODE=simple python inject_red.py )
    echo "== [extended] build extended Red + Blue ROMs =="
    ( cd "$POKERED" && "$MGW/mingw32-make.exe" RGBDS="$RGBDS" pokered.gbc pokeblue.gbc -j4 )
    CRC_R=$(python -c "import zlib;print(f'0x{zlib.crc32(open(r\"$POKERED/pokered.gbc\",\"rb\").read())&0xffffffff:08X}u')")
    CRC_B=$(python -c "import zlib;print(f'0x{zlib.crc32(open(r\"$POKERED/pokeblue.gbc\",\"rb\").read())&0xffffffff:08X}u')")
    echo "   ext Red CRC=$CRC_R  ext Blue CRC=$CRC_B"
    python - "$P/extras.c" "$CRC_R" "$CRC_B" <<'PY'
import re,sys
f,cr,cb=sys.argv[1:]
s=open(f).read()
s=re.sub(r'0x[0-9A-Fa-f]+u(,\s*/\* Pokemon Red Extended)',  cr+r'\1', s)
s=re.sub(r'0x[0-9A-Fa-f]+u(,\s*/\* Pokemon Blue Extended)', cb+r'\1', s)
open(f,'w').write(s)
PY
    cp "$POKERED/pokered.gbc"  "$P/roms/Pokemon_Red_Extended.gb"
    cp "$POKERED/pokeblue.gbc" "$P/roms/Pokemon_Blue_Extended.gb"
    echo "== [extended] static recompile + native build =="
    ( cd "$P" && "$GBRECOMP" --config pokemon_red_extended.toml )
    cp "$POKERED/pokered.gbc" "$P/generated_ext/build/rom_extended.gb" 2>/dev/null || true
    "$MGW/cmake.exe" -G Ninja -S "$P/generated_ext" -B "$P/generated_ext/build"
    "$MGW/ninja.exe" -C "$P/generated_ext/build"
    echo "BUILT: generated_ext/build/Pokemon_Red_Extended.exe (runs ext Red + Blue)"
    "$P/scripts/package.sh" red  extended "$VER"
    "$P/scripts/package.sh" blue extended "$VER"
    ;;
  *) echo "variant must be stock|extended"; exit 1;;
esac
echo "DONE: $V $VER"
