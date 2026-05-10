#include "loogal.h"
#include "cobj.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static void usage(void) {
    puts("CONTINUITY OBJECTS");
    puts("==============================");
    puts("loogal cobj create <id>");
    puts("loogal cobj add <id> <kind> <ref>");
    puts("loogal cobj absorb <id> <source.jsonl> <needle>");
    puts("loogal cobj trace <id>");
}

static void make_object_dir(const char *id, char *dir, size_t dir_sz) {
    snprintf(dir, dir_sz, "continuity/objects/%s", id);
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
}

static int append_manifest(const char *id, const char *line) {
    char dir[1024];
    char manifest[1024];
    make_object_dir(id, dir, sizeof(dir));
    snprintf(manifest, sizeof(manifest), "%s/manifest.jsonl", dir);

    FILE *f = fopen(manifest, "a");
    if (!f) {
        loogal_die("cobj", "could not open manifest");
        return 1;
    }

    fputs(line, f);
    if (line[0] && line[strlen(line) - 1] != '\n') fputc('\n', f);
    fclose(f);
    return 0;
}

static int create_object(const char *id) {
    char record[2048];
    snprintf(record, sizeof(record),
        "{\"type\":\"continuity.object\","
        "\"schema\":\"locus.cobj:v1\","
        "\"id\":\"%s\","
        "\"created_unix\":%ld,"
        "\"status\":\"alive\"}",
        id, now_unix());

    if (append_manifest(id, record) != 0) return 1;

    printf("CONTINUITY OBJECT CREATED\n");
    printf("id:       %s\n", id);
    printf("manifest: continuity/objects/%s/manifest.jsonl\n", id);
    loogal_log("cobj.create", "ok", id);
    return 0;
}

static int add_ref(const char *id, const char *kind, const char *ref) {
    char record[4096];
    snprintf(record, sizeof(record),
        "{\"type\":\"continuity.object.ref\","
        "\"schema\":\"locus.cobj.ref:v1\","
        "\"object\":\"%s\","
        "\"kind\":\"%s\","
        "\"ref\":\"%s\","
        "\"created_unix\":%ld}",
        id, kind, ref, now_unix());

    if (append_manifest(id, record) != 0) return 1;

    printf("CONTINUITY REF ADDED\n");
    printf("object: %s\n", id);
    printf("kind:   %s\n", kind);
    printf("ref:    %s\n", ref);
    loogal_log("cobj.add", "ok", kind);
    return 0;
}

static int absorb_records(const char *id, const char *source, const char *needle) {
    FILE *in = fopen(source, "r");
    if (!in) {
        loogal_die("cobj", "could not open source jsonl");
        return 1;
    }

    char line[16384];
    long copied = 0;

    while (fgets(line, sizeof(line), in)) {
        if (!strstr(line, needle)) continue;
        if (append_manifest(id, line) == 0) copied++;
    }

    fclose(in);

    printf("CONTINUITY RECORDS ABSORBED\n");
    printf("object:  %s\n", id);
    printf("source:  %s\n", source);
    printf("needle:  %s\n", needle);
    printf("copied:  %ld\n", copied);
    loogal_log("cobj.absorb", "ok", id);
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

    while (fgets(line, sizeof(line), f)) fputs(line, stdout);

    fclose(f);
    loogal_log("cobj.trace", "ok", id);
    return 0;
}

int cmd_cobj(int argc, char **argv) {
    if (argc < 1) { usage(); return 1; }

    if (strcmp(argv[0], "create") == 0) {
        if (argc < 2) { loogal_die("cobj", "missing object id"); return 1; }
        return create_object(argv[1]);
    }

    if (strcmp(argv[0], "add") == 0) {
        if (argc < 4) { loogal_die("cobj", "usage: cobj add <id> <kind> <ref>"); return 1; }
        return add_ref(argv[1], argv[2], argv[3]);
    }

    if (strcmp(argv[0], "absorb") == 0) {
        if (argc < 4) { loogal_die("cobj", "usage: cobj absorb <id> <source.jsonl> <needle>"); return 1; }
        return absorb_records(argv[1], argv[2], argv[3]);
    }

    if (strcmp(argv[0], "trace") == 0) {
        if (argc < 2) { loogal_die("cobj", "missing object id"); return 1; }
        return trace_object(argv[1]);
    }

    usage();
    return 1;
}
