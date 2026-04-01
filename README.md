# PokemonRedGBRecomp

Static recompilation of Pokemon Red (Game Boy) for native PC.
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

## Quick Start

1. Clone with submodules: `git clone --recursive https://github.com/mstan/PokemonRedGBRecomp`
2. Place your `Pokemon Red (UE) [S][!].gb` ROM in `roms/`
3. Build and run:

```bash
./build.sh all   # Full build (recompiler + code gen + compile)
./build.sh run   # Build + launch
```

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
git clone --recursive https://github.com/mstan/PokemonRedGBRecomp
cd PokemonRedGBRecomp
cp /path/to/Pokemon\ Red\ \(UE\)\ \[S\]\[\!\].gb roms/
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

- `pokemon_red.toml` — recompiler configuration (entry points, data regions)
- `build.sh` — build script (regen, compile, run)
- `generated/` — auto-generated C code (not committed)
- `gb-recompiled/` — framework submodule (recompiler + runtime)

## Known Limitations

- Only tested through Viridian City area; later game areas may have dispatch misses
- Minor audio stutter during NPC dialogue (LCD-off VRAM updates)
- No link cable support
