#include "loogal.h"
#include "rel.h"

#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("CONTINUITY RELATIONSHIPS");
    puts("==============================");
    puts("loogal rel add <source> <relationship> <target>");
    puts("loogal rel trace <source>");
}

static int rel_add(const char *src, const char *kind, const char *dst) {
    ensure_dirs();

    FILE *f = fopen("continuity/relationships.jsonl", "a");
    if (!f) {
        loogal_die("rel", "could not open relationship database");
        return 1;
    }

    fprintf(f,
        "{\"type\":\"continuity.relationship\","
        "\"schema\":\"locus.rel:v1\","
        "\"source\":\"%s\","
        "\"relationship\":\"%s\","
        "\"target\":\"%s\","
        "\"created_unix\":%ld}"
        "\n",
        src,
        kind,
        dst,
        now_unix());

    fclose(f);

    puts("RELATIONSHIP CREATED");
    puts("==============================");
    printf("source:       %s\n", src);
    printf("relationship: %s\n", kind);
    printf("target:       %s\n", dst);
    printf("database:     continuity/relationships.jsonl\n");

    loogal_log("rel.add", "ok", kind);

    return 0;
}

static int rel_trace(const char *src) {
    FILE *f = fopen("continuity/relationships.jsonl", "r");
    if (!f) {
        loogal_die("rel", "relationship database missing");
        return 1;
    }

    char line[8192];
    long matches = 0;

    puts("CONTINUITY RELATION TRACE");
    puts("==============================");
    printf("source: %s\n\n", src);

    while (fgets(line, sizeof(line), f)) {
        if (!strstr(line, src)) continue;

        matches++;
        puts(line);
    }

    fclose(f);

    printf("relationships: %ld\n", matches);

    loogal_log("rel.trace", "ok", src);

    return 0;
}

int cmd_rel(int argc, char **argv) {
    if (argc < 1) {
        usage();
        return 1;
    }

    if (strcmp(argv[0], "add") == 0) {
        if (argc < 4) {
            loogal_die("rel", "usage: rel add <source> <relationship> <target>");
            return 1;
        }

        return rel_add(argv[1], argv[2], argv[3]);
    }

    if (strcmp(argv[0], "trace") == 0) {
        if (argc < 2) {
            loogal_die("rel", "missing source object");
            return 1;
        }

        return rel_trace(argv[1]);
    }

    usage();
    return 1;
}
