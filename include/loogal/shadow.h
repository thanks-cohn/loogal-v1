#ifndef LOOGAL_SHADOW_H
#define LOOGAL_SHADOW_H

#include <stdint.h>

typedef struct {
    uint64_t dhash64;
    uint64_t ahash64;
    int width;
    int height;
    float aspect;
    int has_dhash64;
    int has_ahash64;
} LoogalShadow;

int loogal_shadow_hamming64(uint64_t a, uint64_t b);
double loogal_shadow_similarity64(uint64_t a, uint64_t b);
double loogal_shadow_aspect_similarity(float a, float b);
double loogal_shadow_fused_score(const LoogalShadow *a, const LoogalShadow *b);

#endif
