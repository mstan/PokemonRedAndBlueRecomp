/*
 * extras.c — PokemonRedAndBlueRecomp game-specific hooks.
 *
 * The runtime library (gbrt) provides weak defaults for every game_*
 * hook in runtime/src/game_extras_default.c. We override only the ROM
 * identity hooks here; everything else stays default.
 *
 * Modeled after FaxanaduRecomp/extras.c in the NES recomp project.
 */
#include <stdint.h>
#include "game_extras.h"

/* Multi-version build: Red and Blue share identical engine code,
 * only the data tables (Pokemon stats, dialog, maps) differ. The
 * launcher accepts either CRC and feeds the chosen ROM into the same
 * recompiled binary. */
const uint32_t *game_get_valid_crcs(int *out_count) {
    static const uint32_t crcs[] = {
        0x9F7FDD53u,  /* Pokemon Red (UE) [S][!] */
        0xD6DA8A1Au,  /* Pokemon Blue (UE) [S][!] */
        0x1C8CD388u,  /* Pokemon Red Extended (Gen2 graft) */
    };
    if (out_count) *out_count = (int)(sizeof(crcs) / sizeof(crcs[0]));
    return crcs;
}

/* Multi-version path is preferred; this stays zero. */
uint32_t game_get_expected_crc32(void) { return 0; }
