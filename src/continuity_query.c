#include "continuity_core.h"
#include "loogal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char line[16384];
    double score;
} ContinuityHit;

static int cmp_hits(const void *a, const void *b) {
    const ContinuityHit *x = (const ContinuityHit *)a;
    const ContinuityHit *y = (const ContinuityHit *)b;

    if (x->score < y->score) return 1;
    if (x->score > y->score) return -1;
    return 0;
}

int continuity_query_manifestations(
    const char *manifest_path,
    uint64_t query_dhash,
    ContinuityHit *hits,
    int max_hits
) {
    FILE *f = fopen(manifest_path, "r");
    if (!f) return 0;

    char line[16384];
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        if (!strstr(line, "continuity.manifestation")) {
            continue;
        }

        char *dhash_pos = strstr(line, "dhash");
        if (!dhash_pos) continue;

        unsigned long long dhash = 0;
        sscanf(dhash_pos, "dhash\\\":\\\"%llx", &dhash);

        double score = continuity_score(
            query_dhash,
            (uint64_t)dhash,
            1.0,
            1.0
        );

        strncpy(hits[count].line, line, sizeof(hits[count].line) - 1);
        hits[count].score = score;

        count++;

        if (count >= max_hits) break;
    }

    fclose(f);

    qsort(hits, count, sizeof(ContinuityHit), cmp_hits);

    return count;
}
