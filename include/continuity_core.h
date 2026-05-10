#ifndef LOOGAL_CONTINUITY_CORE_H
#define LOOGAL_CONTINUITY_CORE_H

#include <stdint.h>

typedef struct {
    char id[128];
    uint64_t created_unix;
} ContinuityObject;

typedef struct {
    char object[128];
    char path[1024];
    uint64_t dhash;
    uint64_t ahash;
    int width;
    int height;
} ManifestationRecord;

typedef struct {
    char source[128];
    char relationship[64];
    char target[128];
} ContinuityRelationship;

double continuity_score(
    uint64_t dhash_a,
    uint64_t dhash_b,
    double geometry_overlap,
    double lineage_weight
);

#endif
