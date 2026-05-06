#include "loogal/shadow.h"

#include <stddef.h>

int loogal_shadow_hamming64(uint64_t a, uint64_t b) {
    uint64_t x = a ^ b;

#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#else
    int count = 0;

    while (x) {
        count += (int)(x & 1ULL);
        x >>= 1;
    }

    return count;
#endif
}

double loogal_shadow_similarity64(uint64_t a, uint64_t b) {
    int d = loogal_shadow_hamming64(a, b);

    if (d < 0) d = 0;
    if (d > 64) d = 64;

    return 1.0 - ((double)d / 64.0);
}

double loogal_shadow_aspect_similarity(float a, float b) {
    if (a <= 0.0f || b <= 0.0f) return 0.0;

    double da = (double)a;
    double db = (double)b;

    double small = da < db ? da : db;
    double large = da > db ? da : db;

    if (large <= 0.0) return 0.0;

    return small / large;
}

double loogal_shadow_fused_score(const LoogalShadow *a, const LoogalShadow *b) {
    if (!a || !b) return 0.0;

    double total_weight = 0.0;
    double score = 0.0;

    if (a->has_dhash64 && b->has_dhash64) {
        score += loogal_shadow_similarity64(a->dhash64, b->dhash64) * 0.70;
        total_weight += 0.70;
    }

    if (a->has_ahash64 && b->has_ahash64) {
        score += loogal_shadow_similarity64(a->ahash64, b->ahash64) * 0.25;
        total_weight += 0.25;
    }

    if (a->aspect > 0.0f && b->aspect > 0.0f) {
        score += loogal_shadow_aspect_similarity(a->aspect, b->aspect) * 0.05;
        total_weight += 0.05;
    }

    if (total_weight <= 0.0) return 0.0;

    return score / total_weight;
}
