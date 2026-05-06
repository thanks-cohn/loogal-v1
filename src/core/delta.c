#include "loogal/delta.h"

#include <string.h>

int loogal_popcount64(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(value);
#else
    int count = 0;
    while (value) {
        count += (int)(value & 1ULL);
        value >>= 1;
    }
    return count;
#endif
}

uint64_t loogal_delta_xor64(uint64_t canonical, uint64_t variant) {
    return canonical ^ variant;
}

uint64_t loogal_delta_apply_xor64(uint64_t canonical, uint64_t delta) {
    return canonical ^ delta;
}

static void fill_bit_positions(uint64_t delta, LoogalDelta64 *out) {
    out->bit_count = 0;

    for (uint8_t i = 0; i < 64; i++) {
        uint64_t mask = 1ULL << i;

        if (delta & mask) {
            out->bits[out->bit_count++] = i;
        }
    }
}

static uint64_t hydrate_from_bit_positions(uint64_t canonical, const LoogalDelta64 *delta) {
    uint64_t mask = 0;

    if (!delta) return canonical;

    for (uint8_t i = 0; i < delta->bit_count && i < 64; i++) {
        if (delta->bits[i] < 64) {
            mask |= (1ULL << delta->bits[i]);
        }
    }

    return canonical ^ mask;
}

int loogal_delta64_make(uint64_t canonical, uint64_t variant, LoogalDelta64 *out) {
    if (!out) return -1;

    memset(out, 0, sizeof(*out));

    uint64_t delta = loogal_delta_xor64(canonical, variant);
    int distance = loogal_popcount64(delta);

    if (distance < 0) return -1;
    if (distance > 64) distance = 64;

    out->distance = (uint8_t)distance;

    if (distance == 0) {
        out->mode = LOOGAL_DELTA_XOR_U64;
        out->value = 0;
        out->bit_count = 0;
        return 0;
    }

    /*
       Storage policy v1:

       Tiny changes use bit positions.
       This is compact and human-inspectable in JSONL.

       Larger changes use XOR delta.
       This is still reversible and simple.

       If the delta becomes so noisy that it no longer represents
       a meaningful near-variant, higher layers may choose to store
       a full hash or promote the variant into a new identity.
    */
    if (distance <= 8) {
        out->mode = LOOGAL_DELTA_BITPOS_U64;
        out->value = 0;
        fill_bit_positions(delta, out);
        return 0;
    }

    out->mode = LOOGAL_DELTA_XOR_U64;
    out->value = delta;
    out->bit_count = 0;

    return 0;
}

uint64_t loogal_delta64_hydrate(uint64_t canonical, const LoogalDelta64 *delta) {
    if (!delta) return canonical;

    switch (delta->mode) {
        case LOOGAL_DELTA_FULL_U64:
            return delta->value;

        case LOOGAL_DELTA_XOR_U64:
            return canonical ^ delta->value;

        case LOOGAL_DELTA_BITPOS_U64:
            return hydrate_from_bit_positions(canonical, delta);

        default:
            return canonical;
    }
}

const char *loogal_delta_mode_name(LoogalDeltaMode mode) {
    switch (mode) {
        case LOOGAL_DELTA_FULL_U64:
            return "full_u64";
        case LOOGAL_DELTA_XOR_U64:
            return "xor_u64";
        case LOOGAL_DELTA_BITPOS_U64:
            return "bitpos_u64";
        default:
            return "unknown";
    }
}
