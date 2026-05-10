#include "loogal.h"
#include "region_search.h"

#include <stdio.h>

int cmd_region_search(int argc, char **argv) {
    if (argc < 1) {
        loogal_die(
            "region-search",
            "usage: loogal region-search <crop.png> [place]"
        );

        return 1;
    }

    const char *query = argv[0];
    const char *place = argc >= 2 ? argv[1] : "memory";

    puts("REGION SEARCH");
    puts("==============================");
    printf("query: %s\n", query);
    printf("scope: %s\n", place);
    puts("");

    puts("region engine: shardscan:v0");
    puts("status: experimental continuity recovery");
    puts("");

    puts("future capabilities:");
    puts("  - image-inside-image recovery");
    puts("  - screenshot ancestry");
    puts("  - exact bbox landing");
    puts("  - pdf manifestation search");
    puts("  - comic panel continuity");
    puts("");

    puts("goal:");
    puts("  tiny crop -> original manifestation");

    loogal_log("region-search.complete", "ok", query);

    return 0;
}
