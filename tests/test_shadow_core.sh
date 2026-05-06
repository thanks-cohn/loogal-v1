#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cat > /tmp/loogal_shadow_test.c <<'C_EOF'
#include "loogal/shadow.h"

#include <stdio.h>
#include <stdint.h>

static int fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

static int approx(double value, double expected, double tolerance) {
    double d = value - expected;
    if (d < 0.0) d = -d;
    return d <= tolerance;
}

int main(void) {
    uint64_t base = 0xffff0000ffff0000ULL;
    uint64_t one_bit = base ^ (1ULL << 3);
    uint64_t four_bits = base ^ 0x000000000000000fULL;

    if (loogal_shadow_hamming64(base, base) != 0) {
        return fail("exact hamming should be 0");
    }

    if (loogal_shadow_hamming64(base, one_bit) != 1) {
        return fail("one-bit hamming should be 1");
    }

    if (loogal_shadow_hamming64(base, four_bits) != 4) {
        return fail("four-bit hamming should be 4");
    }

    if (!approx(loogal_shadow_similarity64(base, base), 1.0, 0.000001)) {
        return fail("exact similarity should be 1.0");
    }

    if (!approx(loogal_shadow_similarity64(base, one_bit), 63.0 / 64.0, 0.000001)) {
        return fail("one-bit similarity mismatch");
    }

    if (!approx(loogal_shadow_aspect_similarity(1.0f, 1.0f), 1.0, 0.000001)) {
        return fail("same aspect should score 1.0");
    }

    if (!approx(loogal_shadow_aspect_similarity(1.0f, 2.0f), 0.5, 0.000001)) {
        return fail("aspect ratio similarity mismatch");
    }

    LoogalShadow a = {
        .dhash64 = base,
        .ahash64 = base,
        .width = 64,
        .height = 64,
        .aspect = 1.0f,
        .has_dhash64 = 1,
        .has_ahash64 = 1
    };

    LoogalShadow b = {
        .dhash64 = one_bit,
        .ahash64 = four_bits,
        .width = 64,
        .height = 64,
        .aspect = 1.0f,
        .has_dhash64 = 1,
        .has_ahash64 = 1
    };

    double fused = loogal_shadow_fused_score(&a, &b);

    if (fused <= 0.90 || fused > 1.0) {
        return fail("fused score should be high for close shadows");
    }

    printf("SHADOW CORE TEST OK\n");
    printf("  hamming exact : %d\n", loogal_shadow_hamming64(base, base));
    printf("  hamming near  : %d\n", loogal_shadow_hamming64(base, one_bit));
    printf("  fused score   : %.6f\n", fused);

    return 0;
}
C_EOF

cc -O2 -Wall -Wextra -std=c11 -Iinclude \
  /tmp/loogal_shadow_test.c src/core/shadow.c \
  -o /tmp/loogal_shadow_test

/tmp/loogal_shadow_test
