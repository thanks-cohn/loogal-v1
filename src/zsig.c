#include "loogal.h"
#include "zsig.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint64_t fnv1a_u64(const char *s) {
    uint64_t h = 1469598103934665603ULL;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

static void usage(void) {
    puts("ZONE SIGNATURES");
    puts("==============================");
    puts("loogal zsig <image> --box x,y,w,h [--shape box|polygon]");
    puts("");
    puts("Deterministic region signatures for future continuity search.");
}

int cmd_zsig(int argc, char **argv) {
    if (argc < 1) {
        usage();
        return 1;
    }

    const char *image = argv[0];
    const char *box = NULL;
    const char *shape = "box";

    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--box") == 0) box = argv[i + 1];
        if (strcmp(argv[i], "--shape") == 0) shape = argv[i + 1];
    }

    if (!box) {
        loogal_die("zsig", "missing --box x,y,w,h");
        return 1;
    }

    char canonical[4096];
    snprintf(canonical, sizeof(canonical), "%s|%s|%s", image, shape, box);

    uint64_t sig = fnv1a_u64(canonical);

    ensure_dirs();

    FILE *f = fopen("data/zone_signatures.jsonl", "a");
    if (!f) {
        loogal_die("zsig", "could not open signature database");
        return 1;
    }

    fprintf(f,
        "{\"type\":\"zone.signature\","
        "\"schema\":\"locus.zsig:v1\","
        "\"image\":\"%s\","
        "\"shape\":\"%s\","
        "\"box\":\"%s\","
        "\"signature\":\"%016llx\","
        "\"created_unix\":%ld}"
        "\n",
        image,
        shape,
        box,
        (unsigned long long)sig,
        now_unix());

    fclose(f);

    puts("ZONE SIGNATURE GENERATED");
    puts("==============================");
    printf("image:      %s\n", image);
    printf("shape:      %s\n", shape);
    printf("region:     %s\n", box);
    printf("signature:  %016llx\n", (unsigned long long)sig);
    printf("database:   data/zone_signatures.jsonl\n");

    loogal_log("zsig", "ok", canonical);

    return 0;
}
