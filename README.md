# PokemonRedAndBlueRecomp

Static recompilation of Pokemon Red and Blue (Game Boy) for native PC.
Built with the [gb-recompiled](https://github.com/mstan/gbrecompiled) framework.

> **Status: Early — playable through Viridian City.** Title screen, intro, overworld, NPC dialogue, and Pokemon Center healing all work. No known dispatch misses on the tested path. If you find a bug, please open an issue.

## What Works

- Title screen, NEW GAME / OPTIONS menu
- Overworld exploration (Pallet Town, Route 1, Viridian City)
- NPC dialogue and interactions
- Pokemon Center healing sequence
- Saving/loading SRAM to disk
- Audio (music and SFX)
- Input recording/playback (`--record` / `--script`)
- Both Pokemon Red and Pokemon Blue ROMs supported

## Quick Start

1. Clone with submodules: `git clone --recursive https://github.com/mstan/PokemonRedAndBlueRecomp`
2. Place your ROM in `roms/` — either:
   - `Pokemon Red (UE) [S][!].gb`
   - `Pokemon - Blue Version (UE) [S][!].gb`
3. Build and run:

```bash
./build.sh all   # Full build (recompiler + code gen + compile)
./build.sh run   # Build + launch
```

The game will prompt you to select a ROM on first launch. Both Red and Blue are accepted.

## Controls

| GB Button | Keyboard |
|-----------|----------|
| D-Pad     | Arrow keys |
| A         | Z |
| B         | X |
| Start     | Enter |
| Select    | Tab |

| Hotkey | Action |
|--------|--------|
| TAB (hold) | Turbo (fast-forward) |

## Building from Source

Requires MSYS2 MinGW64 toolchain, CMake 3.20+, Ninja, and SDL2.

```bash
git clone --recursive https://github.com/mstan/PokemonRedAndBlueRecomp
cd PokemonRedAndBlueRecomp
cp /path/to/your-pokemon-rom.gb roms/
./build.sh all
./build.sh run
```

Build commands:
- `./build.sh all` — Full rebuild (recompiler + regenerate + compile)
- `./build.sh game` — Incremental compile only
- `./build.sh regen` — Regenerate C code from ROM
- `./build.sh bank 00 03` — Regenerate + rebuild specific banks
- `./build.sh run` — Build + launch

## Architecture

This is a **static recompiler**, not an emulator. The original SM83 (Game Boy CPU) machine code is translated to C at build time, then compiled to native x64. The Game Boy PPU, APU, and memory mapper are simulated by the runtime library.

Red and Blue share identical code layout — only data tables differ (Pokemon availability, title screen, etc.). The same recompiled code works with either ROM loaded at runtime.

- `pokemon_red_blue.toml` — recompiler configuration (entry points, data regions, valid CRCs)
- `build.sh` — build script (regen, compile, run)
- `generated/` — auto-generated C code (not committed)
- `gb-recompiled/` — framework submodule (recompiler + runtime)

## Known Limitations

- Only tested through Viridian City area; later game areas may have dispatch misses
- Minor audio stutter during NPC dialogue (LCD-off VRAM updates)
- No link cable support
