#include "continuity_core.h"

#include <stdint.h>

static int popcount64(uint64_t x) {
    int c = 0;
    while (x) {
        c += (x & 1ULL);
        x >>= 1ULL;
    }
    return c;
}

double continuity_score(
    uint64_t dhash_a,
    uint64_t dhash_b,
    double geometry_overlap,
    double lineage_weight
) {
    uint64_t delta = dhash_a ^ dhash_b;
    int dist = popcount64(delta);

    double visual = 1.0 - ((double)dist / 64.0);

    double score =
        (visual * 0.70) +
        (geometry_overlap * 0.20) +
        (lineage_weight * 0.10);

    if (score < 0.0) score = 0.0;
    if (score > 1.0) score = 1.0;

    return score;
}
