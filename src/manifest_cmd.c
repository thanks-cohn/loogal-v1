#include "loogal.h"
#include "manifest_cmd.h"

#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("CONTINUITY MANIFESTATIONS");
    puts("==============================");
    puts("loogal manifest add <object> <path>");
    puts("loogal manifest trace <object>");
}

static int manifest_add(const char *object, const char *path) {
    LoogalImageInfo info;
    memset(&info, 0, sizeof(info));

    if (image_probe(path, &info) != 0) {
        loogal_die("manifest", "could not probe manifestation image");
        return 1;
    }

    char manifest_path[1024];
    snprintf(
        manifest_path,
        sizeof(manifest_path),
        "continuity/objects/%s/manifest.jsonl",
        object);

    FILE *f = fopen(manifest_path, "a");
    if (!f) {
        loogal_die("manifest", "object manifest missing");
        return 1;
    }

    fprintf(f,
        "{\"type\":\"continuity.manifestation\","
        "\"schema\":\"locus.manifest:v1\","
        "\"object\":\"%s\","
        "\"path\":\"%s\","
        "\"width\":%d,"
        "\"height\":%d,"
        "\"dhash\":\"%016llx\","
        "\"ahash\":\"%016llx\","
        "\"created_unix\":%ld}"
        "\n",
        object,
        path,
        info.width,
        info.height,
        (unsigned long long)info.dhash,
        (unsigned long long)info.ahash,
        now_unix());

    fclose(f);

    puts("MANIFESTATION RECORDED");
    puts("==============================");
    printf("object:  %s\n", object);
    printf("path:    %s\n", path);
    printf("dhash:   %016llx\n", (unsigned long long)info.dhash);
    printf("ahash:   %016llx\n", (unsigned long long)info.ahash);

    loogal_log("manifest.add", "ok", object);

    return 0;
}

static int manifest_trace(const char *object) {
    char manifest_path[1024];
    snprintf(
        manifest_path,
        sizeof(manifest_path),
        "continuity/objects/%s/manifest.jsonl",
        object);

    FILE *f = fopen(manifest_path, "r");
    if (!f) {
        loogal_die("manifest", "object manifest missing");
        return 1;
    }

    char line[16384];

    puts("MANIFESTATION TRACE");
    puts("==============================");
    printf("object: %s\n\n", object);

    while (fgets(line, sizeof(line), f)) {
        if (!strstr(line, "continuity.manifestation")) continue;
        fputs(line, stdout);
    }

    fclose(f);

    loogal_log("manifest.trace", "ok", object);

    return 0;
}

int cmd_manifest(int argc, char **argv) {
    if (argc < 1) {
        usage();
        return 1;
    }

    if (strcmp(argv[0], "add") == 0) {
        if (argc < 3) {
            loogal_die("manifest", "usage: manifest add <object> <path>");
            return 1;
        }

        return manifest_add(argv[1], argv[2]);
    }

    if (strcmp(argv[0], "trace") == 0) {
        if (argc < 2) {
            loogal_die("manifest", "missing object id");
            return 1;
        }

        return manifest_trace(argv[1]);
    }

    usage();
    return 1;
}
