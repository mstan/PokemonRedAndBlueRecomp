# Rendering & Audio Improvement Initiative

Tracked improvements for the Pokemon Red/Blue recompiler runtime, ordered by priority.

## High Priority

- [x] **VRAM/OAM write protection during PPU rendering** — `gbrt.c:525-530, 573-579`
  VRAM writes during mode 3 and OAM writes during modes 2-3 are not blocked (commented out), allowing mid-scanline data corruption. Read protection is still enforced, creating an asymmetry. Re-enable write protection to match real hardware behavior.

- [x] **SCX/SCY per-scanline latching** — `ppu.c:492-522`
  Register changes to SCX/SCY apply immediately instead of being latched at the start of each scanline. Pokemon's title screen scrolling effect and battle transitions depend on precise per-line scroll updates. Implement scanline-level latching.

- [x] **LCD on/off transition state reset** — `ppu.c:470-486`
  When LCD is re-enabled, PPU doesn't reset to OAM mode (mode 2) to start a fresh frame cycle. Should reset mode, LY, and cycle counter properly. Causes rendering artifacts after LCD off/on sequences used heavily in battle intros.

- [x] **Variable mode 3 (pixel transfer) timing** — `ppu.h:26`
  `CYCLES_PIXEL_DRAW` is hardcoded to 172, but real hardware varies (171-289) based on sprite count, SCX fine scroll, and window state. Battle scenes with many sprites have incorrect STAT timing.

## Medium Priority

- [ ] **Frame-ready flag race condition** — `ppu.c:393-396`
  If `frame_ready` isn't cleared by the platform in time, the next frame render is skipped. Can cause dropped frames during animation-heavy sequences. (Attempted fix caused visual regression — needs different approach.)

- [ ] **Audio sample dropping** — `platform_sdl.cpp:183-186`
  When SDL audio queue exceeds 16KB (~46ms), entire batch is silently discarded. Should always queue audio and use vsync for backpressure. (Buffer tuning caused instability — needs deeper investigation.)

- [x] **Audio sample rate drift** — `audio.c:168`
  Already fixed in code via 16.16 fixed-point timing (SAMPLE_PERIOD_FIXED = 6233460).

## Low Priority

- [x] **Channel 4 (noise) LFSR excessive iteration** — `audio.c:493-517`
  Capped at LFSR cycle length (127 for 7-bit, 32767 for 15-bit).

- [ ] **VSync threshold mismatch** — `platform_sdl.cpp:561-579`
  vsync wait threshold (8192 bytes) and queue drop threshold (16384 bytes) create an unstable window.

- [x] **Window line counter mid-frame WY changes** — `ppu.c:133-195`
  Window now triggers on exact LY==WY match, not retroactively.

- [x] **convert_to_rgb debug overhead** — `ppu.c:284-301`
  Removed dbg_has_nonzero_pixels (23K byte scan per frame) and periodic debug logging.
