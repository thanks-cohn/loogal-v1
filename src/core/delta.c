#include "loogal/delta.h"

unsigned loogal_delta_hamming_u64(uint64_t a, uint64_t b) {
    uint64_t x = a ^ b;
    unsigned count = 0;

    while (x) {
        count += (unsigned)(x & 1ULL);
        x >>= 1;
    }

    return count;
}

double loogal_delta_similarity_u64(uint64_t a, uint64_t b) {
    unsigned distance = loogal_delta_hamming_u64(a, b);

    if (distance >= 64) {
        return 0.0;
    }

    return ((64.0 - (double)distance) / 64.0) * 100.0;
}

bool loogal_delta_match_u64(uint64_t a, uint64_t b, double threshold_percent) {
    double similarity = loogal_delta_similarity_u64(a, b);
    return similarity >= threshold_percent;
}

loogal_delta_result loogal_delta_compare_u64(uint64_t a, uint64_t b) {
    loogal_delta_result result;

    result.a = a;
    result.b = b;
    result.distance = loogal_delta_hamming_u64(a, b);
    result.similarity = loogal_delta_similarity_u64(a, b);
    result.exact = result.distance == 0;

    return result;
}
