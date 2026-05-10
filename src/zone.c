#define _POSIX_C_SOURCE 200809L

#include "loogal.h"
#include "jsonout.h"
#include "zone.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOGAL_ZONES_PATH "data/zones.jsonl"
#define LOOGAL_ZONE_MAX_FIELDS 64

typedef struct { const char *key; const char *value; } ZoneField;

static void usage(void) {
    puts("ZONE MEMORY");
    puts("loogal zone add <image> --box x,y,w,h --name <label>");
    puts("  [--category root/branch/leaf|dataset/train/class-a]");
    puts("  [--tags a,b,c] [--clusters x,y,z] [--family name]");
    puts("  [--parent id-or-name] [--child id-or-name]");
    puts("  [--kind person|specimen|artifact|region|custom]");
    puts("  [--description text] [--confidence n] [--field key=value]");
    puts("");
    puts("loogal zone import <file.csv> --format csv [--category path]");
    puts("  CSV columns: image,bbox,name,kind,category,tags,clusters,family,parent,child");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc - 1; i++) if (strcmp(argv[i], flag) == 0) return argv[i + 1];
    return NULL;
}

static int collect_fields(int argc, char **argv, ZoneField *fields, int max_fields) {
    static char keys[LOOGAL_ZONE_MAX_FIELDS][128];
    static char vals[LOOGAL_ZONE_MAX_FIELDS][1024];
    int count = 0;
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], "--field") != 0 || count >= max_fields) continue;
        const char *raw = argv[i + 1];
        const char *eq = strchr(raw, '=');
        if (!eq || eq == raw || !eq[1]) continue;
        size_t n = (size_t)(eq - raw);
        if (n >= sizeof(keys[count])) n = sizeof(keys[count]) - 1;
        memcpy(keys[count], raw, n);
        keys[count][n] = '\0';
        snprintf(vals[count], sizeof(vals[count]), "%s", eq + 1);
        fields[count].key = keys[count];
        fields[count].value = vals[count];
        count++;
    }
    return count;
}

static void js(FILE *f, const char *key, const char *value, int comma) {
    fprintf(f, "\"%s\":", key);
    loogal_json_string(f, value ? value : "");
    if (comma) fputc(',', f);
}

static void fields_json(FILE *f, ZoneField *fields, int n) {
    fputs("\"fields\":{", f);
    for (int i = 0; i < n; i++) js(f, fields[i].key, fields[i].value, i + 1 < n);
    fputc('}', f);
}

static int write_zone_record(FILE *f, const char *record_type, const char *schema,
    unsigned long long zone_id, const char *image, const char *box, const char *name,
    const char *kind, const char *description, const char *category, const char *tags,
    const char *clusters, const char *family, const char *parent, const char *child,
    const char *confidence, const char *source, ZoneField *fields, int field_count)
{
    fputc('{', f);
    js(f, "type", record_type, 1);
    js(f, "schema", schema, 1);
    fprintf(f, "\"zone_id\":%llu,", zone_id);
    fprintf(f, "\"created_unix\":%ld,", now_unix());
    js(f, "image", image, 1);
    js(f, "shape", "box", 1);
    js(f, "bbox", box, 1);
    js(f, "name", name, 1);
    js(f, "kind", kind ? kind : "region", 1);
    js(f, "description", description ? description : "", 1);
    js(f, "category", category ? category : "", 1);
    js(f, "tags", tags ? tags : "", 1);
    js(f, "clusters", clusters ? clusters : "", 1);
    js(f, "family", family ? family : "", 1);
    js(f, "parent", parent ? parent : "", 1);
    js(f, "child", child ? child : "", 1);
    js(f, "confidence", confidence ? confidence : "1.0", 1);
    js(f, "source", source ? source : "manual", 1);
    fields_json(f, fields, field_count);
    fputs("}\n", f);
    return 0;
}

static int append_zone_pair(unsigned long long zone_id, const char *image, const char *box, const char *name,
    const char *kind, const char *description, const char *category, const char *tags, const char *clusters,
    const char *family, const char *parent, const char *child, const char *confidence, const char *source,
    ZoneField *fields, int field_count)
{
    FILE *state = fopen(LOOGAL_ZONES_PATH, "a");
    if (!state) return 1;
    write_zone_record(state, "zone.annotation", "locus.zone:v1", zone_id, image, box, name, kind,
        description, category, tags, clusters, family, parent, child, confidence, source, fields, field_count);
    fclose(state);

    FILE *events = fopen(LOOGAL_EVENTS_PATH, "a");
    if (events) {
        write_zone_record(events, "zone.imported", "locus.event.zone:v1", zone_id, image, box, name, kind,
            description, category, tags, clusters, family, parent, child, confidence, source, fields, field_count);
        fclose(events);
    } else {
        loogal_log("zone.event", "error", "could not append zone event");
    }
    return 0;
}

static void trim_newline(char *s) {
    if (!s) return;
    for (char *p = s; *p; p++) if (*p == '\n' || *p == '\r') { *p = '\0'; return; }
}

static int zone_import_csv(const char *path, const char *override_category) {
    FILE *in = fopen(path, "r");
    if (!in) { loogal_die("zone.import", "could not open import file"); return 1; }

    char line[8192];
    long imported = 0;
    long skipped = 0;

    while (fgets(line, sizeof(line), in)) {
        trim_newline(line);
        if (!line[0] || line[0] == '#') continue;
        if (strncmp(line, "image,", 6) == 0) continue;

        char *cols[10] = {0};
        char *save = NULL;
        char *tok = strtok_r(line, ",", &save);
        int n = 0;
        while (tok && n < 10) { cols[n++] = tok; tok = strtok_r(NULL, ",", &save); }
        if (n < 3 || !cols[0][0] || !cols[1][0] || !cols[2][0]) { skipped++; continue; }

        ZoneField fields[2];
        fields[0].key = "import.format"; fields[0].value = "csv";
        fields[1].key = "edit.status"; fields[1].value = "imported-editable";
        unsigned long long zone_id = (unsigned long long)now_unix() + (unsigned long long)imported;
        append_zone_pair(zone_id, cols[0], cols[1], cols[2], n > 3 ? cols[3] : "region", "",
            override_category ? override_category : (n > 4 ? cols[4] : ""),
            n > 5 ? cols[5] : "", n > 6 ? cols[6] : "", n > 7 ? cols[7] : "",
            n > 8 ? cols[8] : "", n > 9 ? cols[9] : "", "1.0", "import", fields, 2);
        imported++;
    }

    fclose(in);
    printf("ZONES IMPORTED\n");
    printf("file:     %s\n", path);
    printf("imported: %ld\n", imported);
    printf("skipped:  %ld\n", skipped);
    printf("state:    data/zones.jsonl\n");
    printf("events:   data/events.jsonl\n");
    loogal_log("zone.import", "ok", path);
    return 0;
}

int cmd_zone(int argc, char **argv) {
    if (argc < 1) { usage(); return 1; }

    if (strcmp(argv[0], "import") == 0) {
        if (argc < 2) { loogal_die("zone.import", "missing import file"); return 1; }
        const char *format = arg_value(argc, argv, "--format");
        const char *category = arg_value(argc, argv, "--category");
        if (!format || strcmp(format, "csv") == 0) return zone_import_csv(argv[1], category);
        loogal_die("zone.import", "unsupported format; use --format csv");
        return 1;
    }

    if (strcmp(argv[0], "add") != 0) { loogal_die("zone", "supported: add, import"); return 1; }
    if (argc < 2) { loogal_die("zone", "missing image path"); return 1; }

    const char *image = argv[1];
    const char *box = arg_value(argc, argv, "--box");
    const char *name = arg_value(argc, argv, "--name");
    const char *category = arg_value(argc, argv, "--category");
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

    if (!box || !name) { loogal_die("zone", "--box and --name are required"); return 1; }

    unsigned long long zone_id = (unsigned long long)now_unix();
    if (append_zone_pair(zone_id, image, box, name, kind, description, category, tags, clusters,
        family, parent, child, confidence, "manual", fields, field_count) != 0) {
        loogal_die("zone", "could not open zones database"); return 1;
    }

    puts("ZONE REMEMBERED");
    puts("==============================");
    printf("zone_id:     %llu\n", zone_id);
    printf("image:       %s\n", image);
    printf("shape:       box\n");
    printf("bbox:        %s\n", box);
    printf("name:        %s\n", name);
    printf("kind:        %s\n", kind ? kind : "region");
    if (category && *category) printf("category:    %s\n", category);
    if (tags && *tags) printf("tags:        %s\n", tags);
    if (clusters && *clusters) printf("clusters:    %s\n", clusters);
    if (family && *family) printf("family:      %s\n", family);
    if (parent && *parent) printf("parent:      %s\n", parent);
    if (child && *child) printf("child:       %s\n", child);
    if (field_count > 0) printf("fields:      %d custom\n", field_count);
    puts("");
    puts("state:  data/zones.jsonl");
    puts("event:  data/events.jsonl");
    puts("status: continuity witness stored and event appended");
    loogal_log("zone.add", "ok", name);
    return 0;
}
