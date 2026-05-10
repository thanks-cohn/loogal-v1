#include "loogal.h"
#include "zone_search.h"

#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("ZONE SEARCH");
    puts("==============================");
    puts("loogal zone-search <signature>");
    puts("");
    puts("Search continuity region signatures.");
}

int cmd_zone_search(int argc, char **argv) {
    if (argc < 1) {
        usage();
        return 1;
    }

    const char *target = argv[0];

    FILE *f = fopen("data/zone_signatures.jsonl", "r");
    if (!f) {
        loogal_die("zone-search", "missing zone signature database");
        return 1;
    }

    char line[8192];
    long matches = 0;

    puts("ZONE CONTINUITY SEARCH");
    puts("==============================");
    printf("query: %s\n", target);
    puts("");

    while (fgets(line, sizeof(line), f)) {
        if (!strstr(line, target)) continue;

        matches++;

        printf("MATCH %ld\n", matches);
        puts("------------------------------");
        puts(line);
    }

    fclose(f);

    printf("total_matches: %ld\n", matches);

    if (matches == 0) {
        puts("status: no manifestations recovered");
    } else {
        puts("status: manifestations recovered");
    }

    loogal_log("zone-search", "ok", target);

    return 0;
}
