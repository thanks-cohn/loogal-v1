#include "continuity_core.h"
#include "loogal.h"

#include <stdio.h>
#include <string.h>

int continuity_graph_neighbors(
    const char *object,
    char neighbors[][128],
    int max_neighbors
) {
    FILE *f = fopen("continuity/relationships.jsonl", "r");
    if (!f) return 0;

    char line[16384];
    int count = 0;

    while (fgets(line, sizeof(line), f)) {
        if (!strstr(line, object)) continue;

        char target[128] = {0};

        char *t = strstr(line, "target");
        if (!t) continue;

        sscanf(t, "target\":\"%127[^\"]", target);

        if (target[0]) {
            strncpy(neighbors[count], target, 127);
            count++;
        }

        if (count >= max_neighbors) break;
    }

    fclose(f);

    return count;
}

int continuity_graph_walk(
    const char *root,
    int depth
) {
    char neighbors[256][128];

    int count = continuity_graph_neighbors(
        root,
        neighbors,
        256
    );

    puts("CONTINUITY GRAPH WALK");
    puts("==============================");
    printf("root:  %s\n", root);
    printf("depth: %d\n\n", depth);

    for (int i = 0; i < count; i++) {
        printf("[%d] %s\n", i + 1, neighbors[i]);
    }

    loogal_log("continuity.graph.walk", "ok", root);

    return count;
}
