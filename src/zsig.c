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
    puts("Creates a continuity witness for a bounded region.");
    puts("V1 combines image perceptual witnesses with canonical geometry.");
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

    LoogalImageInfo info;
    memset(&info, 0, sizeof(info));

    if (image_probe(image, &info) != 0) {
        loogal_die("zsig", "could not probe source image");
        return 1;
    }

    char canonical[4096];
    snprintf(
        canonical,
        sizeof(canonical),
        "%s|%s|%s|%dx%d|%016llx|%016llx",
        image,
        shape,
        box,
        info.width,
        info.height,
        (unsigned long long)info.dhash,
        (unsigned long long)info.ahash
    );

    uint64_t sig = fnv1a_u64(canonical);
    uint64_t geom_sig = fnv1a_u64(box);

    ensure_dirs();

    FILE *f = fopen("data/zone_signatures.jsonl", "a");
    if (!f) {
        loogal_die("zsig", "could not open signature database");
        return 1;
    }

    fprintf(f,
        "{\"type\":\"zone.signature\","
        "\"schema\":\"locus.zsig:v2\","
        "\"image\":\"%s\","
        "\"shape\":\"%s\","
        "\"box\":\"%s\","
        "\"image_width\":%d,"
        "\"image_height\":%d,"
        "\"image_dhash\":\"%016llx\","
        "\"image_ahash\":\"%016llx\","
        "\"geometry_signature\":\"%016llx\","
        "\"signature\":\"%016llx\","
        "\"created_unix\":%ld}"
        "\n",
        image,
        shape,
        box,
        info.width,
        info.height,
        (unsigned long long)info.dhash,
        (unsigned long long)info.ahash,
        (unsigned long long)geom_sig,
        (unsigned long long)sig,
        now_unix());

    fclose(f);

    puts("ZONE SIGNATURE GENERATED");
    puts("==============================");
    printf("image:       %s\n", image);
    printf("shape:       %s\n", shape);
    printf("region:      %s\n", box);
    printf("dimensions:  %dx%d\n", info.width, info.height);
    printf("dhash:       %016llx\n", (unsigned long long)info.dhash);
    printf("ahash:       %016llx\n", (unsigned long long)info.ahash);
    printf("geometry:    %016llx\n", (unsigned long long)geom_sig);
    printf("signature:   %016llx\n", (unsigned long long)sig);
    printf("database:    data/zone_signatures.jsonl\n");

    loogal_log("zsig", "ok", canonical);

    return 0;
}
