#ifndef LOOGAL_DELTA_H
#define LOOGAL_DELTA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t a;
    uint64_t b;
    unsigned distance;
    double similarity;
    bool exact;
} loogal_delta_result;

unsigned loogal_delta_hamming_u64(uint64_t a, uint64_t b);

double loogal_delta_similarity_u64(uint64_t a, uint64_t b);

bool loogal_delta_match_u64(uint64_t a, uint64_t b, double threshold_percent);

loogal_delta_result loogal_delta_compare_u64(uint64_t a, uint64_t b);

#ifdef __cplusplus
}
#endif

#endif
