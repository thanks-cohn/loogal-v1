#include "why.h"
#include "jsonout.h"
#include "loogal.h"
#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int line_has_path(const char *line, const char *path) {
    return line && path && strstr(line, path) != NULL;
}

static int extract_string_field(const char *line, const char *key, char *out, size_t out_sz) {
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\":\"", key);

    const char *p = strstr(line, needle);
    if (!p) return 0;

    p += strlen(needle);

    const char *end = strchr(p, '"');
    if (!end) return 0;

    size_t n = (size_t)(end - p);
    if (n >= out_sz) n = out_sz - 1;

    memcpy(out, p, n);
    out[n] = '\0';

    return 1;
}

static int extract_ull_field(const char *line, const char *key, unsigned long long *out) {
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\":", key);

    const char *p = strstr(line, needle);
    if (!p) return 0;

    p += strlen(needle);

    while (*p == ' ' || *p == '\t') p++;

    char *end = NULL;
    unsigned long long value = strtoull(p, &end, 10);

    if (end == p) return 0;

    *out = value;
    return 1;
}

static int find_identity_by_sha256(const char *sha256,
                                   unsigned long long *identity_id,
                                   char *identity_line,
                                   size_t identity_line_sz) {
    FILE *f = fopen(LOOGAL_IDENTITIES_PATH, "r");
    if (!f) return -1;

    char line[8192];

    while (fgets(line, sizeof(line), f)) {
        char found_sha[128];

        if (!extract_string_field(line, "sha256", found_sha, sizeof(found_sha))) {
            continue;
        }

        if (strcmp(found_sha, sha256) != 0) {
            continue;
        }

        if (!extract_ull_field(line, "id", identity_id)) {
            fclose(f);
            return -1;
        }

        if (identity_line && identity_line_sz > 0) {
            snprintf(identity_line, identity_line_sz, "%s", line);
        }

        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}

static long count_locations_for_identity(unsigned long long identity_id) {
    FILE *f = fopen(LOOGAL_LOCATIONS_PATH, "r");
    if (!f) return -1;

    long count = 0;
    char line[8192];

    while (fgets(line, sizeof(line), f)) {
        unsigned long long found_id = 0;

        if (!extract_ull_field(line, "identity_id", &found_id)) {
            continue;
        }

        if (found_id == identity_id) {
            count++;
        }
    }

    fclose(f);
    return count;
}

static int print_locations_text(unsigned long long identity_id) {
    FILE *f = fopen(LOOGAL_LOCATIONS_PATH, "r");
    if (!f) return -1;

    char line[8192];
    long n = 0;

    while (fgets(line, sizeof(line), f)) {
        unsigned long long found_id = 0;
        char path[LOOGAL_PATH_MAX];

        if (!extract_ull_field(line, "identity_id", &found_id)) {
            continue;
        }

        if (found_id != identity_id) {
            continue;
        }

        path[0] = '\0';
        extract_string_field(line, "path", path, sizeof(path));

        n++;

        if (path[0]) {
            printf("  %ld. %s\n", n, path);
        } else {
            printf("  %ld. %s", n, line);
            if (line[strlen(line) - 1] != '\n') puts("");
        }
    }

    fclose(f);
    return 0;
}

static int print_locations_json(unsigned long long identity_id) {
    FILE *f = fopen(LOOGAL_LOCATIONS_PATH, "r");
    if (!f) return -1;

    char line[8192];
    int first = 1;

    puts("  \"locations\": [");

    while (fgets(line, sizeof(line), f)) {
        unsigned long long found_id = 0;
        char path[LOOGAL_PATH_MAX];

        if (!extract_ull_field(line, "identity_id", &found_id)) {
            continue;
        }

        if (found_id != identity_id) {
            continue;
        }

        path[0] = '\0';
        extract_string_field(line, "path", path, sizeof(path));

        if (!first) puts(",");
        printf("    {\"path\": \"%s\"}", path);

        first = 0;
    }

    puts("");
    puts("  ]");

    fclose(f);
    return 0;
}

int cmd_why(int argc, char **argv) {
    if (argc < 1) {
        puts("LOOGAL WHY");
        puts("Usage:");
        puts("  loogal why <path> [--json]");
        loogal_log("why", "error", "missing path");
        return 1;
    }

    const char *target = argv[0];
    int as_json = loogal_has_flag(argc, argv, "--json");

    FILE *f = fopen("data/records.jsonl", "r");
    if (!f) {
        puts("LOOGAL WHY");
        puts("[ERR] missing data/records.jsonl");
        puts("Run:");
        puts("  loogal index <directories...>");
        loogal_log("why", "error", "records file missing");
        return 1;
    }

    char line[4096];
    char found[4096];
    found[0] = '\0';

    while (fgets(line, sizeof(line), f)) {
        if (line_has_path(line, target)) {
            snprintf(found, sizeof(found), "%s", line);
            break;
        }
    }

    fclose(f);

    if (found[0] == '\0') {
        if (as_json) {
            puts("{");
            printf("  ");
            loogal_json_kv_string(stdout, "status", "not_found", 1);
            printf("  ");
            loogal_json_kv_string(stdout, "path", target, 0);
            puts("}");
        } else {
            puts("LOOGAL WHY");
            puts("────────────────────────");
            printf("Path: %s\n", target);
            puts("Status: not found in Loogal records");
            puts("");
            puts("Try:");
            puts("  loogal index <directory-containing-this-file>");
        }

        loogal_log("why", "miss", target);
        return 1;
    }

    if (as_json) {
        puts("{");
        printf("  ");
        loogal_json_kv_string(stdout, "status", "found", 1);
        printf("  ");
        loogal_json_kv_string(stdout, "path", target, 1);
        printf("  ");
        loogal_json_kv_string(stdout, "record_jsonl", found, 0);
        puts("}");
    } else {
        puts("LOOGAL WHY");
        puts("────────────────────────");
        printf("Path: %s\n", target);
        puts("Status: found in visual memory");
        puts("");
        puts("Known because:");
        puts("  indexed into data/records.jsonl");
        puts("  represented in data/loogal.bin");
        puts("  searchable by dHash v1 appearance signature");
        puts("");
        puts("Raw record:");
        fputs(found, stdout);
    }

    loogal_log("why", "ok", target);
    return 0;
}

int cmd_where(int argc, char **argv) {
    if (argc < 1) {
        puts("LOOGAL WHERE");
        puts("Usage:");
        puts("  loogal where <query-image> [--json]");
        loogal_log("where", "error", "missing query image");
        return 1;
    }

    const char *query_path = argv[0];
    int as_json = loogal_has_flag(argc, argv, "--json");

    char sha256[65];

    if (loogal_sha256_file(query_path, sha256) != 0 || !sha256[0]) {
        if (as_json) {
            puts("{");
            printf("  ");
            loogal_json_kv_string(stdout, "status", "error", 1);
            printf("  ");
            loogal_json_kv_string(stdout, "reason", "could_not_hash_query", 1);
            printf("  ");
            loogal_json_kv_string(stdout, "query", query_path, 0);
            puts("}");
        } else {
            puts("LOOGAL WHERE");
            puts("────────────────────────");
            printf("Query: %s\n", query_path);
            puts("Status: error");
            puts("Reason: could not compute sha256 for query image");
        }

        loogal_log("where", "error", query_path);
        return 1;
    }

    unsigned long long identity_id = 0;
    char identity_line[8192];
    identity_line[0] = '\0';

    int find_rc = find_identity_by_sha256(sha256, &identity_id, identity_line, sizeof(identity_line));

    if (find_rc < 0) {
        if (as_json) {
            puts("{");
            printf("  ");
            loogal_json_kv_string(stdout, "status", "error", 1);
            printf("  ");
            loogal_json_kv_string(stdout, "reason", "missing_identity_memory", 1);
            printf("  ");
            loogal_json_kv_string(stdout, "identities_path", LOOGAL_IDENTITIES_PATH, 0);
            puts("}");
        } else {
            puts("LOOGAL WHERE");
            puts("────────────────────────");
            puts("[ERR] missing identity memory");
            puts("Run:");
            puts("  loogal index <directories...>");
        }

        loogal_log("where", "error", "identity memory missing");
        return 1;
    }

    if (find_rc > 0) {
        if (as_json) {
            puts("{");
            printf("  ");
            loogal_json_kv_string(stdout, "status", "not_found", 1);
            printf("  ");
            loogal_json_kv_string(stdout, "query", query_path, 1);
            printf("  ");
            loogal_json_kv_string(stdout, "sha256", sha256, 0);
            puts("}");
        } else {
            puts("LOOGAL WHERE");
            puts("────────────────────────");
            printf("Query: %s\n", query_path);
            printf("sha256: %s\n", sha256);
            puts("Status: not found in Loogal identity memory");
            puts("");
            puts("Try:");
            puts("  loogal index <directory-containing-this-image>");
        }

        loogal_log("where", "miss", query_path);
        return 1;
    }

    long location_count = count_locations_for_identity(identity_id);
    if (location_count < 0) location_count = 0;

    if (as_json) {
        puts("{");
        printf("  ");
        loogal_json_kv_string(stdout, "status", "found", 1);
        printf("  ");
        loogal_json_kv_string(stdout, "query", query_path, 1);
        printf("  ");
        loogal_json_kv_string(stdout, "sha256", sha256, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "identity_id", (long long)identity_id, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "location_count", location_count, 1);
        print_locations_json(identity_id);
        puts("}");
    } else {
        puts("LOOGAL WHERE");
        puts("────────────────────────");
        printf("Query: %s\n", query_path);
        printf("Status: found\n");
        printf("Identity ID: %llu\n", identity_id);
        printf("sha256: %s\n", sha256);
        printf("Locations: %ld\n", location_count);
        puts("");
        puts("Known locations:");
        print_locations_text(identity_id);
        puts("");
        puts("Interpretation:");
        puts("  This query image maps to one visual identity.");
        puts("  Every listed path is a known location for that identity.");
    }

    loogal_log("where", "ok", query_path);
    return 0;
}
