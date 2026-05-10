#include "continuity_core.h"
#include "loogal.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint64_t dhash;
    uint64_t offset;
} ManifestationIndexRecord;

int continuity_build_manifestation_index(
    const char *manifest_path,
    const char *index_path
) {
    FILE *in = fopen(manifest_path, "r");
    if (!in) return 1;

    FILE *out = fopen(index_path, "wb");
    if (!out) {
        fclose(in);
        return 1;
    }

    char line[16384];

    while (fgets(line, sizeof(line), in)) {
        if (!strstr(line, "continuity.manifestation")) {
            continue;
        }

        char *dhash_pos = strstr(line, "dhash");
        if (!dhash_pos) continue;

        unsigned long long dhash = 0;
        sscanf(dhash_pos, "dhash\\\":\\\"%llx", &dhash);

        ManifestationIndexRecord rec;
        rec.dhash = (uint64_t)dhash;
        rec.offset = (uint64_t)ftell(in);

        fwrite(&rec, sizeof(rec), 1, out);
    }

    fclose(in);
    fclose(out);

    loogal_log("continuity.index", "ok", manifest_path);

    return 0;
}
