#include "loogal.h"
#include "cobj.h"

#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("CONTINUITY OBJECTS");
    puts("==============================");
    puts("loogal cobj create <id>");
    puts("loogal cobj trace <id>");
}

static int create_object(const char *id) {
    char dir[1024];
    snprintf(dir, sizeof(dir), "continuity/objects/%s", id);

    ensure_dirs();

#ifdef _WIN32
    mkdir("continuity");
    mkdir("continuity/objects");
    mkdir(dir);
#else
    mkdir("continuity", 0755);
    mkdir("continuity/objects", 0755);
    mkdir(dir, 0755);
#endif

    char manifest[1024];
    snprintf(manifest, sizeof(manifest), "%s/manifest.jsonl", dir);

    FILE *f = fopen(manifest, "a");
    if (!f) {
        loogal_die("cobj", "could not create manifest");
        return 1;
    }

    fprintf(f,
        "{\"type\":\"continuity.object\","
        "\"schema\":\"locus.cobj:v1\","
        "\"id\":\"%s\","
        "\"created_unix\":%ld,"
        "\"status\":\"alive\"}"
        "\n",
        id,
        now_unix());

    fclose(f);

    printf("CONTINUITY OBJECT CREATED\n");
    printf("id:       %s\n", id);
    printf("manifest: %s\n", manifest);

    loogal_log("cobj.create", "ok", id);

    return 0;
}

static int trace_object(const char *id) {
    char manifest[1024];
    snprintf(manifest, sizeof(manifest), "continuity/objects/%s/manifest.jsonl", id);

    FILE *f = fopen(manifest, "r");
    if (!f) {
        loogal_die("cobj", "manifest missing");
        return 1;
    }

    char line[8192];

    puts("CONTINUITY TRACE");
    puts("==============================");
    printf("object: %s\n\n", id);

    while (fgets(line, sizeof(line), f)) {
        puts(line);
    }

    fclose(f);

    loogal_log("cobj.trace", "ok", id);

    return 0;
}

int cmd_cobj(int argc, char **argv) {
    if (argc < 1) {
        usage();
        return 1;
    }

    if (strcmp(argv[0], "create") == 0) {
        if (argc < 2) {
            loogal_die("cobj", "missing object id");
            return 1;
        }

        return create_object(argv[1]);
    }

    if (strcmp(argv[0], "trace") == 0) {
        if (argc < 2) {
            loogal_die("cobj", "missing object id");
            return 1;
        }

        return trace_object(argv[1]);
    }

    usage();
    return 1;
}
