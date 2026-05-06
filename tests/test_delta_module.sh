#!/usr/bin/env bash
set -euo pipefail

cc="${CC:-gcc}"

tmp_c="$(mktemp /tmp/loogal_delta_test.XXXXXX.c)"
tmp_bin="$(mktemp /tmp/loogal_delta_test.XXXXXX)"

cleanup() {
  rm -f "$tmp_c" "$tmp_bin"
}
trap cleanup EXIT

cat > "$tmp_c" <<'C_EOF'
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "loogal/delta.h"

static int almost_equal(double a, double b) {
    double d = a - b;
    if (d < 0) d = -d;
    return d < 0.0001;
}

int main(void) {
    uint64_t same_a = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t same_b = 0xAAAAAAAAAAAAAAAAULL;

    uint64_t one_bit_a = 0xAAAAAAAAAAAAAAAAULL;
    uint64_t one_bit_b = 0xAAAAAAAAAAAAAAABULL;

    uint64_t many_a = 0x0000000000000000ULL;
    uint64_t many_b = 0xFFFFFFFFFFFFFFFFULL;

    unsigned d0 = loogal_delta_hamming_u64(same_a, same_b);
    if (d0 != 0) {
        fprintf(stderr, "exact distance failed: got %u\n", d0);
        return 1;
    }
    printf("[delta] exact match passed\n");

    unsigned d1 = loogal_delta_hamming_u64(one_bit_a, one_bit_b);
    if (d1 != 1) {
        fprintf(stderr, "one-bit distance failed: got %u\n", d1);
        return 1;
    }
    printf("[delta] one-bit distance passed\n");

    unsigned d64 = loogal_delta_hamming_u64(many_a, many_b);
    if (d64 != 64) {
        fprintf(stderr, "many-bit distance failed: got %u\n", d64);
        return 1;
    }
    printf("[delta] many-bit distance passed\n");

    double exact_sim = loogal_delta_similarity_u64(same_a, same_b);
    if (!almost_equal(exact_sim, 100.0)) {
        fprintf(stderr, "exact similarity failed: got %.4f\n", exact_sim);
        return 1;
    }
    printf("[delta] exact similarity passed\n");

    double one_bit_sim = loogal_delta_similarity_u64(one_bit_a, one_bit_b);
    if (!almost_equal(one_bit_sim, 98.4375)) {
        fprintf(stderr, "one-bit similarity failed: got %.4f\n", one_bit_sim);
        return 1;
    }
    printf("[delta] one-bit similarity passed\n");

    bool should_match = loogal_delta_match_u64(one_bit_a, one_bit_b, 98.0);
    if (!should_match) {
        fprintf(stderr, "threshold match failed\n");
        return 1;
    }
    printf("[delta] threshold match passed\n");

    bool should_not_match = loogal_delta_match_u64(many_a, many_b, 1.0);
    if (should_not_match) {
        fprintf(stderr, "threshold rejection failed\n");
        return 1;
    }
    printf("[delta] threshold rejection passed\n");

    loogal_delta_result r = loogal_delta_compare_u64(one_bit_a, one_bit_b);
    if (r.distance != 1 || r.exact || !almost_equal(r.similarity, 98.4375)) {
        fprintf(stderr, "delta result struct failed\n");
        return 1;
    }
    printf("[delta] result struct passed\n");

    printf("[delta] all tests passed\n");
    return 0;
}
C_EOF

"$cc" -std=c11 -Wall -Wextra -Werror \
  -Iinclude \
  "$tmp_c" src/core/delta.c \
  -o "$tmp_bin"

"$tmp_bin"
