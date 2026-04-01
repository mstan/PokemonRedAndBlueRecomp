# Rendering & Audio Improvement Initiative

Tracked improvements for the Pokemon Red/Blue recompiler runtime, ordered by priority.

## High Priority

- [ ] **VRAM/OAM write protection during PPU rendering** — `gbrt.c:525-530, 573-579`
  VRAM writes during mode 3 and OAM writes during modes 2-3 are not blocked (commented out), allowing mid-scanline data corruption. Read protection is still enforced, creating an asymmetry. Re-enable write protection to match real hardware behavior.

- [ ] **SCX/SCY per-scanline latching** — `ppu.c:492-522`
  Register changes to SCX/SCY apply immediately instead of being latched at the start of each scanline. Pokemon's title screen scrolling effect and battle transitions depend on precise per-line scroll updates. Implement scanline-level latching.

- [ ] **LCD on/off transition state reset** — `ppu.c:470-486`
  When LCD is re-enabled, PPU doesn't reset to OAM mode (mode 2) to start a fresh frame cycle. Should reset mode, LY, and cycle counter properly. Causes rendering artifacts after LCD off/on sequences used heavily in battle intros.

- [ ] **Variable mode 3 (pixel transfer) timing** — `ppu.h:26`
  `CYCLES_PIXEL_DRAW` is hardcoded to 172, but real hardware varies (171-289) based on sprite count, SCX fine scroll, and window state. Battle scenes with many sprites have incorrect STAT timing.

## Medium Priority

- [ ] **Frame-ready flag race condition** — `ppu.c:393-396`
  If `frame_ready` isn't cleared by the platform in time, the next frame render is skipped. Can cause dropped frames during animation-heavy sequences.

- [ ] **Audio sample dropping** — `platform_sdl.cpp:183-186`
  When SDL audio queue exceeds 16KB (~46ms), entire batch is silently discarded. Should always queue audio and use vsync for backpressure.

- [ ] **Audio sample rate drift** — `audio.c:168`
  Using 95 cycles/sample instead of 95.108 causes audio to run ~0.1% fast. Over 10 seconds, ~44 extra samples accumulate. Use fixed-point timing.

## Low Priority

- [ ] **Channel 4 (noise) LFSR excessive iteration** — `audio.c:493-517`
  Tight loop can iterate thousands of times when noise frequency is high. Pre-calculate iteration count and use bit manipulation.

- [ ] **VSync threshold mismatch** — `platform_sdl.cpp:561-579`
  vsync wait threshold (8192 bytes) and queue drop threshold (16384 bytes) create an unstable window.

- [ ] **Window line counter mid-frame WY changes** — `ppu.c:133-195`
  Window internal line counter doesn't handle WY register changes mid-frame. Only matters for games that modify WY outside VBlank.
