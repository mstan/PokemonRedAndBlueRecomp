# PokemonRedAndBlueRecomp

A static recompilation of **Pokemon Red** and **Pokemon Blue** (Game Boy) for native PC.

This project uses [gb-recompiled](https://github.com/mstan/gbrecompiled) (forked from [arcanite24/gb-recompiled](https://github.com/arcanite24/gb-recompiled)) to translate the original Game Boy machine code into C, which is then compiled to a native x64 executable. The Game Boy's PPU, APU, and memory mapper are handled by the gb-recompiled runtime library.

[![Pokemon Red/Blue Recompiled - Demo](https://img.youtube.com/vi/pEgJCTf7kgY/0.jpg)](https://www.youtube.com/watch?v=pEgJCTf7kgY)

## Status

**Early working prototype.** The game is playable from the title screen through the Elite Four, but this is not a finished product.

- The game runs and is playable with both Red and Blue ROMs
- Some animations may lag or chug slightly (e.g., certain battle intro spirals)
- Minor visual glitches may occur in edge cases
- Code paths not yet discovered by the static analyzer will fall back to an interpreter — this works but is slower than recompiled code
- Audio works but may stutter briefly during animation-heavy scenes
- No link cable support

If you encounter a crash or a new interpreter fallback, please open an issue with the address logged to the console.

## Quick Start

### Pre-built Release

1. Download the latest release from the [Releases](https://github.com/mstan/PokemonRedAndBlueRecomp/releases) page
2. Extract the zip
3. Run `Pokemon_Red_Blue.exe`
4. Select your ROM when prompted — either Pokemon Red or Pokemon Blue (UE) is accepted

### Building from Source

Requires MSYS2 MinGW64 toolchain, CMake 3.20+, Ninja, and SDL2.

```bash
git clone --recursive https://github.com/mstan/PokemonRedAndBlueRecomp
cd PokemonRedAndBlueRecomp
cp /path/to/your-pokemon-rom.gb roms/
./build.sh all   # Full build (recompiler + code gen + compile)
./build.sh run   # Build + launch
```

Build commands:
- `./build.sh all` — Full rebuild (recompiler + regenerate + compile)
- `./build.sh game` — Incremental compile only
- `./build.sh regen` — Regenerate C code from ROM
- `./build.sh bank 00 03` — Regenerate + rebuild specific banks
- `./build.sh run` — Build + launch

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

## How It Works

This is a **static recompiler**, not an emulator. The original SM83 (Game Boy CPU) machine code is translated to C at build time, then compiled to native x64. At runtime, the executable loads the original ROM file for its data (graphics, maps, text, Pokemon stats, etc.) while executing the recompiled native code.

Red and Blue share identical code layout — only data tables differ (Pokemon availability, title screen graphics, etc.). A single recompiled build accepts either ROM at runtime via CRC32 validation.

When the recompiler encounters a code path it didn't discover during static analysis, it falls back to an interpreter. These fallback addresses can be added to the configuration file (`pokemon_red_blue.toml`) to be included in the next build.

### Project Structure

- `pokemon_red_blue.toml` — Recompiler configuration (entry points, data regions, valid CRCs)
- `build.sh` — Build script (regen, compile, run)
- `generated/` — Auto-generated C code from the recompiler (not committed)
- `gb-recompiled/` — Framework submodule ([mstan/gbrecompiled](https://github.com/mstan/gbrecompiled))
- `INITIATIVE.md` — Tracked rendering and audio improvement work

## Credits

- **[arcanite24/gb-recompiled](https://github.com/arcanite24/gb-recompiled)** — The original static recompilation framework that this project is built on. All recompiler and runtime code is derived from this project.
- **[pret/pokered](https://github.com/pret/pokered)** — The pokered disassembly project, used as a reference for entry points and symbol information.

## Known Limitations

- Some animation-heavy scenes (e.g., certain battle intro effects) may experience frame drops
- Audio may stutter briefly during heavy rendering
- Undiscovered code paths fall back to interpreter (logged to console)
- No link cable or printer support
- Only tested with US/Europe ROM versions

## Legal

This project does not include any Nintendo copyrighted material. You must provide your own legally obtained Pokemon Red or Blue ROM file. The generated C code is not committed to this repository.

---

<p align="center">
  <sub><b>R.A.I.D. — Retro AI Development</b> · a Discord for AI-assisted retro reverse-engineering, decomp &amp; recomp</sub>
</p>

<p align="center">
  <a href="https://discord.gg/Ad9BwSzctP"><img src=".github/raid-discord.png" alt="Join the Retro AI Development (R.A.I.D.) Discord" width="200"></a>
</p>
