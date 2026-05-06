#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cat > /tmp/loogal_delta_test.c <<'C_EOF'
#include "loogal/delta.h"

#include <stdio.h>
#include <stdint.h>

static int fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

int main(void) {
    uint64_t canonical = 0xf0f0f0f0f0f0f0f0ULL;
    uint64_t variant_one_bit = canonical ^ (1ULL << 7);
    uint64_t variant_many_bits = canonical ^ 0x00ff00ff00ff00ffULL;

    LoogalDelta64 d1;
    LoogalDelta64 d2;

    if (loogal_delta64_make(canonical, variant_one_bit, &d1) != 0) {
        return fail("could not make one-bit delta");
    }

    if (d1.distance != 1) {
        return fail("one-bit delta distance should be 1");
    }

    if (d1.mode != LOOGAL_DELTA_BITPOS_U64) {
        return fail("one-bit delta should use bit positions");
    }

    if (loogal_delta64_hydrate(canonical, &d1) != variant_one_bit) {
        return fail("one-bit hydration mismatch");
    }

    if (loogal_delta64_make(canonical, variant_many_bits, &d2) != 0) {
        return fail("could not make many-bit delta");
    }

    if (d2.distance <= 8) {
        return fail("many-bit delta distance unexpectedly small");
    }

    if (d2.mode != LOOGAL_DELTA_XOR_U64) {
        return fail("many-bit delta should use xor mode");
    }

    if (loogal_delta64_hydrate(canonical, &d2) != variant_many_bits) {
        return fail("many-bit hydration mismatch");
    }

    printf("DELTA MODULE TEST OK\n");
    printf("  one-bit mode : %s distance=%u\n", loogal_delta_mode_name(d1.mode), d1.distance);
    printf("  many-bit mode: %s distance=%u\n", loogal_delta_mode_name(d2.mode), d2.distance);

    return 0;
}
C_EOF

cc -O2 -Wall -Wextra -std=c11 -Iinclude \
  /tmp/loogal_delta_test.c src/core/delta.c \
  -o /tmp/loogal_delta_test

/tmp/loogal_delta_test
