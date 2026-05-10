#include "loogal.h"
#include "jsonout.h"
#include "zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOGAL_ZONES_PATH "data/zones.jsonl"
#define LOOGAL_ZONE_MAX_FIELDS 64

typedef struct {
    const char *key;
    const char *value;
} ZoneField;

static void usage(void) {
    puts("ZONE MEMORY");
    puts("==============================");
    puts("loogal zone add <image> --box x,y,w,h --name <label>");
    puts("                     [--tags a,b,c]");
    puts("                     [--clusters x,y,z]");
    puts("                     [--family name]");
    puts("                     [--parent id-or-name]");
    puts("                     [--child id-or-name]");
    puts("                     [--kind person|specimen|artifact|region|custom]");
    puts("                     [--field key=value]");
    puts("");
    puts("Advanced shape-ready schema:");
    puts("  V1 stores shape=box and bbox=x,y,w,h.");
    puts("  Future shape=polygon can store points=x1,y1|x2,y2|...");
    puts("  Future shape=mask can store mask_ref=<path-or-id>.");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            return argv[i + 1];
        }
    }

    return NULL;
}

static int collect_fields(int argc, char **argv, ZoneField *fields, int max_fields) {
    int count = 0;

    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "--field") != 0) continue;
        if (count >= max_fields) break;

        const char *raw = argv[i + 1];
        const char *eq = strchr(raw, '=');

        if (!eq || eq == raw || eq[1] == '\0') continue;

        static char keys[LOOGAL_ZONE_MAX_FIELDS][128];
        static char vals[LOOGAL_ZONE_MAX_FIELDS][1024];

        size_t key_len = (size_t)(eq - raw);
        if (key_len >= sizeof(keys[count])) key_len = sizeof(keys[count]) - 1;

        memcpy(keys[count], raw, key_len);
        keys[count][key_len] = '\0';

        snprintf(vals[count], sizeof(vals[count]), "%s", eq + 1);

        fields[count].key = keys[count];
        fields[count].value = vals[count];
        count++;
    }

    return count;
}

static void write_json_string_field(FILE *f, const char *key, const char *value, int comma) {
    fprintf(f, "\"%s\":", key);
    loogal_json_string(f, value ? value : "");
    if (comma) fputc(',', f);
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
    const char *family = arg_value(argc, argv, "--family");
    const char *parent = arg_value(argc, argv, "--parent");
    const char *child = arg_value(argc, argv, "--child");
    const char *kind = arg_value(argc, argv, "--kind");
    const char *description = arg_value(argc, argv, "--description");
    const char *confidence = arg_value(argc, argv, "--confidence");

    ZoneField fields[LOOGAL_ZONE_MAX_FIELDS];
    int field_count = collect_fields(argc, argv, fields, LOOGAL_ZONE_MAX_FIELDS);

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

    fputc('{', f);
    write_json_string_field(f, "type", "zone.annotation", 1);
    write_json_string_field(f, "schema", "locus.zone:v1", 1);
    fprintf(f, "\"zone_id\":%llu,", zone_id);
    write_json_string_field(f, "image", image, 1);
    write_json_string_field(f, "shape", "box", 1);
    write_json_string_field(f, "bbox", box, 1);
    write_json_string_field(f, "name", name, 1);
    write_json_string_field(f, "kind", kind ? kind : "region", 1);
    write_json_string_field(f, "description", description ? description : "", 1);
    write_json_string_field(f, "tags", tags ? tags : "", 1);
    write_json_string_field(f, "clusters", clusters ? clusters : "", 1);
    write_json_string_field(f, "family", family ? family : "", 1);
    write_json_string_field(f, "parent", parent ? parent : "", 1);
    write_json_string_field(f, "child", child ? child : "", 1);
    write_json_string_field(f, "confidence", confidence ? confidence : "1.0", 1);
    fprintf(f, "\"fields\":{");

    for (int i = 0; i < field_count; i++) {
        write_json_string_field(f, fields[i].key, fields[i].value, i + 1 < field_count);
    }

    fprintf(f, "}}");
    fputc('\n', f);

    fclose(f);

    puts("ZONE REMEMBERED");
    puts("==============================");
    printf("zone_id:     %llu\n", zone_id);
    printf("image:       %s\n", image);
    printf("shape:       box\n");
    printf("bbox:        %s\n", box);
    printf("name:        %s\n", name);
    printf("kind:        %s\n", kind ? kind : "region");

    if (tags && *tags) printf("tags:        %s\n", tags);
    if (clusters && *clusters) printf("clusters:    %s\n", clusters);
    if (family && *family) printf("family:      %s\n", family);
    if (parent && *parent) printf("parent:      %s\n", parent);
    if (child && *child) printf("child:       %s\n", child);
    if (field_count > 0) printf("fields:      %d custom\n", field_count);

    puts("");
    puts("status: continuity witness stored");

    loogal_log("zone.add", "ok", name);

    return 0;
}
