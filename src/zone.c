#include "loogal.h"
#include "zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOGAL_ZONES_PATH "data/zones.jsonl"

static void usage(void) {
    puts("ZONE MEMORY");
    puts("==============================");
    puts("loogal zone add <image> --box x,y,w,h --name <label>");
    puts("                     [--tags a,b,c]");
    puts("                     [--clusters x,y,z]");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return argv[i + 1];
        }
    }

    return NULL;
}

int cmd_zone(int argc, char **argv) {
    if (argc < 1) {
        usage();
        return 1;
    }

    const char *sub = argv[0];

    if (strcmp(sub, "add") != 0) {
        loogal_die("zone", "only 'add' currently supported");
        return 1;
    }

    if (argc < 2) {
        loogal_die("zone", "missing image path");
        return 1;
    }

    const char *image = argv[1];
    const char *box = arg_value(argc, argv, "--box");
    const char *name = arg_value(argc, argv, "--name");
    const char *tags = arg_value(argc, argv, "--tags");
    const char *clusters = arg_value(argc, argv, "--clusters");

    if (!box || !name) {
        loogal_die("zone", "--box and --name are required");
        return 1;
    }

    FILE *f = fopen(LOOGAL_ZONES_PATH, "a");

    if (!f) {
        loogal_die("zone", "could not open zones database");
        return 1;
    }

    unsigned long long zone_id = (unsigned long long)now_unix();

    fprintf(
        f,
        "{"
        "\"type\":\"zone.annotation\"," 
        "\"zone_id\":%llu,"
        "\"image\":\"%s\","
        "\"box\":\"%s\","
        "\"name\":\"%s\","
        "\"tags\":\"%s\","
        "\"clusters\":\"%s\""
        "}\n",
        zone_id,
        image,
        box,
        name,
        tags ? tags : "",
        clusters ? clusters : ""
    );

    fclose(f);

    puts("ZONE REMEMBERED");
    puts("==============================");
    printf("zone_id:   %llu\n", zone_id);
    printf("image:     %s\n", image);
    printf("box:       %s\n", box);
    printf("name:      %s\n", name);

    if (tags && *tags) {
        printf("tags:      %s\n", tags);
    }

    if (clusters && *clusters) {
        printf("clusters:  %s\n", clusters);
    }

    puts("");
    puts("status: continuity witness stored");

    loogal_log("zone.add", "ok", name);

    return 0;
}
