#ifndef LOOGAL_DELTA_H
#define LOOGAL_DELTA_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    LOOGAL_DELTA_FULL_U64 = 1,
    LOOGAL_DELTA_XOR_U64 = 2,
    LOOGAL_DELTA_BITPOS_U64 = 3
} LoogalDeltaMode;

typedef struct {
    LoogalDeltaMode mode;
    uint64_t value;
    uint8_t distance;
    uint8_t bit_count;
    uint8_t bits[64];
} LoogalDelta64;

int loogal_popcount64(uint64_t value);
uint64_t loogal_delta_xor64(uint64_t canonical, uint64_t variant);
uint64_t loogal_delta_apply_xor64(uint64_t canonical, uint64_t delta);
int loogal_delta64_make(uint64_t canonical, uint64_t variant, LoogalDelta64 *out);
uint64_t loogal_delta64_hydrate(uint64_t canonical, const LoogalDelta64 *delta);
const char *loogal_delta_mode_name(LoogalDeltaMode mode);

#endif
