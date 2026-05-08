#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "jsonout.h"
#include "timer.h"
#include "loogal/shadow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    int hash_distance;
    int ahash_distance;
    double similarity;
    double dhash_similarity;
    double ahash_similarity;
    double aspect_similarity;
    LoogalRecord rec;
} Hit;

typedef enum {
    LOOGAL_SEARCH_ENGINE_DHASH = 1,
    LOOGAL_SEARCH_ENGINE_SHADOW = 2
} LoogalSearchEngine;

typedef struct {
    unsigned long long identity_id;
    long location_count;
    char locations[16][LOOGAL_PATH_MAX];
    long stored_locations;
} SearchIdentityInfo;

static int cmp_hit(const void *a, const void *b) {
    const Hit *ha = (const Hit *)a;
    const Hit *hb = (const Hit *)b;

    if (ha->similarity < hb->similarity) return 1;
    if (ha->similarity > hb->similarity) return -1;

    if (ha->hash_distance != hb->hash_distance) {
        return ha->hash_distance - hb->hash_distance;
    }

    if (ha->ahash_distance != hb->ahash_distance) {
        return ha->ahash_distance - hb->ahash_distance;
    }

    return strcmp(ha->rec.path, hb->rec.path);
}

static int is_numberish(const char *s) {
    if (!s || !*s) return 0;

    int dot_seen = 0;

    for (const char *p = s; *p; p++) {
        if (*p == '.') {
            if (dot_seen) return 0;
            dot_seen = 1;
            continue;
        }

        if (!isdigit((unsigned char)*p)) return 0;
    }

    return 1;
}

static int starts_with_path(const char *path, const char *prefix) {
    if (!prefix || !*prefix) return 1;

    size_t n = strlen(prefix);

    if (strncmp(path, prefix, n) != 0) return 0;

    return path[n] == '\0' || path[n] == '/';
}

static void normalize_path_best_effort(const char *in, char *out, size_t n) {
    if (!out || n == 0) return;

    if (!in || !*in) {
        snprintf(out, n, "%s", "");
        return;
    }

    char resolved[PATH_MAX];

    #ifdef _WIN32
    if (_fullpath(resolved, in, sizeof(resolved))) {
        snprintf(out, n, "%s", resolved);
        return;
    }
    #else
    if (realpath(in, resolved)) {
        size_t len = strlen(resolved);

        if (len >= n) len = n - 1;

        memcpy(out, resolved, len);
        out[len] = '\0';
    } else {
        snprintf(out, n, "%s", in);
    }
    #endif
}

static int search_extract_string_field(const char *line, const char *key, char *out, size_t out_sz) {
    char needle[128];

    snprintf(needle, sizeof(needle), "\"%s\":\"", key);

    const char *pos = strstr(line, needle);

    if (!pos) return 0;

    pos += strlen(needle);

    const char *end = strchr(pos, '"');

    if (!end) return 0;

    size_t n = (size_t)(end - pos);

    if (n >= out_sz) n = out_sz - 1;

    memcpy(out, pos, n);
    out[n] = '\0';

    return 1;
}

static int search_extract_ull_field(const char *line, const char *key, unsigned long long *out) {
    char needle[128];

    snprintf(needle, sizeof(needle), "\"%s\":", key);

    const char *pos = strstr(line, needle);

    if (!pos) return 0;

    pos += strlen(needle);

    while (*pos == ' ' || *pos == '\t') pos++;

    char *end = NULL;
    unsigned long long value = strtoull(pos, &end, 10);

    if (end == pos) return 0;

    *out = value;
    return 1;
}

static int search_identity_for_path(const char *path, unsigned long long *identity_id) {
    FILE *f = fopen(LOOGAL_LOCATIONS_PATH, "r");

    if (!f) return -1;

    char line[8192];

    while (fgets(line, sizeof(line), f)) {
        char found_path[LOOGAL_PATH_MAX];
        unsigned long long found_id = 0;

        found_path[0] = '\0';

        if (!search_extract_string_field(line, "path", found_path, sizeof(found_path))) continue;
        if (strcmp(found_path, path) != 0) continue;
        if (!search_extract_ull_field(line, "identity_id", &found_id)) continue;

        *identity_id = found_id;
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}

static SearchIdentityInfo load_identity_info_for_path(const char *path) {
    SearchIdentityInfo info;

    memset(&info, 0, sizeof(info));

    unsigned long long identity_id = 0;
    int rc = search_identity_for_path(path, &identity_id);

    if (rc != 0) {
        return info;
    }

    info.identity_id = identity_id;

    FILE *f = fopen(LOOGAL_LOCATIONS_PATH, "r");

    if (!f) {
        return info;
    }

    char line[8192];

    while (fgets(line, sizeof(line), f)) {
        unsigned long long found_id = 0;
        char found_path[LOOGAL_PATH_MAX];

        found_path[0] = '\0';

        if (!search_extract_ull_field(line, "identity_id", &found_id)) continue;
        if (found_id != identity_id) continue;
        if (!search_extract_string_field(line, "path", found_path, sizeof(found_path))) continue;

        if (info.stored_locations < 16) {
            snprintf(
                info.locations[info.stored_locations],
                sizeof(info.locations[info.stored_locations]),
                "%s",
                found_path
            );
            info.stored_locations++;
        }

        info.location_count++;
    }

    fclose(f);
    return info;
}

static void print_action_json(FILE *out, const char *key, const char *cmd, const char *path, int comma) {
    char action[LOOGAL_PATH_MAX + 128];

    snprintf(action, sizeof(action), "%s --path '%s'", cmd, path);

    fputs("      ", out);
    loogal_json_string(out, key);
    fputs(": ", out);
    loogal_json_string(out, action);

    if (comma) fputc(',', out);

    fputc('\n', out);
}

static void print_identity_locations_json(SearchIdentityInfo *info) {
    puts("      \"locations\": [");

    for (long i = 0; i < info->stored_locations; i++) {
        printf("        {\"path\": ");
        loogal_json_string(stdout, info->locations[i]);
        printf("}%s\n", (i + 1 < info->stored_locations) ? "," : "");
    }

    puts("      ],");
}

static void print_json_results(
    const char *query_path,
    const char *place_filter,
    double min_percent,
    LoogalSearchEngine engine,
    size_t candidates,
    size_t total_hits,
    Hit *hits,
    size_t offset,
    size_t limit,
    double elapsed_ms
) {
    size_t end = offset + limit;

    if (end > total_hits) end = total_hits;

    size_t returned = offset < total_hits ? end - offset : 0;
    double elapsed_s = elapsed_ms / 1000.0;
    double elapsed_ns = elapsed_s * 1000000000.0;
    double images_per_second = elapsed_s > 0.0 ? ((double)candidates / elapsed_s) : 0.0;
    double images_per_ns = elapsed_ns > 0.0 ? ((double)candidates / elapsed_ns) : 0.0;
    double ns_per_image = candidates > 0 ? (elapsed_ns / (double)candidates) : 0.0;

    puts("{");
    printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
    printf("  "); loogal_json_kv_string(stdout, "type", "search.results", 1);
    printf("  "); loogal_json_kv_string(stdout, "engine", engine == LOOGAL_SEARCH_ENGINE_SHADOW ? "binary_index:shadow:v1" : "binary_index:dhash:v1", 1);
    printf("  "); loogal_json_kv_string(stdout, "identity_expansion", "locations.jsonl:v1", 1);
    printf("  "); loogal_json_kv_string(stdout, "query", query_path, 1);

    if (place_filter && *place_filter) {
        printf("  "); loogal_json_kv_string(stdout, "place", place_filter, 1);
    } else {
        printf("  "); loogal_json_kv_string(stdout, "place", "memory", 1);
    }

    printf("  \"min_percent\": %.3f,\n", min_percent);
    printf("  "); loogal_json_kv_int(stdout, "candidates", (long long)candidates, 1);
    printf("  "); loogal_json_kv_int(stdout, "total_hits", (long long)total_hits, 1);
    printf("  "); loogal_json_kv_int(stdout, "offset", (long long)offset, 1);
    printf("  "); loogal_json_kv_int(stdout, "limit", (long long)limit, 1);
    printf("  "); loogal_json_kv_int(stdout, "returned", (long long)returned, 1);
    printf("  \"duration_seconds\": %.9f,\n", elapsed_s);
    printf("  \"duration_nanoseconds\": %.0f,\n", elapsed_ns);
    printf("  \"images_per_second\": %.3f,\n", images_per_second);
    printf("  \"images_per_nanosecond\": %.12f,\n", images_per_ns);
    printf("  \"nanoseconds_per_image\": %.3f,\n", ns_per_image);
    puts("  \"results\": [");

    for (size_t i = offset; i < end; i++) {
        Hit *h = &hits[i];
        double pct = h->similarity * 100.0;
        const char *label = h->hash_distance == 0 ? "EXACT" : "NEAR";
        SearchIdentityInfo identity = load_identity_info_for_path(h->rec.path);

        printf("    {\n");
        printf("      \"rank\": %zu,\n", i + 1);
        printf("      \"path\": "); loogal_json_string(stdout, h->rec.path); puts(",");
        printf("      \"label\": "); loogal_json_string(stdout, label); puts(",");
        printf("      \"similarity\": %.6f,\n", h->similarity);
        printf("      \"similarity_percent\": %.3f,\n", pct);
        printf("      \"hash_distance\": %d,\n", h->hash_distance);
        printf("      \"ahash_distance\": %d,\n", h->ahash_distance);
        printf("      \"dhash_similarity\": %.6f,\n", h->dhash_similarity);
        printf("      \"ahash_similarity\": %.6f,\n", h->ahash_similarity);
        printf("      \"aspect_similarity\": %.6f,\n", h->aspect_similarity);
        printf("      \"width\": %d,\n", h->rec.width);
        printf("      \"height\": %d,\n", h->rec.height);
        printf("      \"file_size_bytes\": %llu,\n", (unsigned long long)h->rec.file_size);

        if (identity.identity_id) {
            printf("      \"identity_id\": %llu,\n", identity.identity_id);
            printf("      \"location_count\": %ld,\n", identity.location_count);
            print_identity_locations_json(&identity);
        } else {
            puts("      \"identity_id\": 0,");
            puts("      \"location_count\": 0,");
            puts("      \"locations\": [],");
        }

        puts("      \"actions\": {");
        print_action_json(stdout, "reveal", "loogal action reveal", h->rec.path, 1);
        print_action_json(stdout, "open", "loogal action open", h->rec.path, 1);
        print_action_json(stdout, "copy_path", "loogal action copy-path", h->rec.path, 1);

        char search_action[LOOGAL_PATH_MAX + 128];

        snprintf(search_action, sizeof(search_action), "loogal window --query '%s' --memory", h->rec.path);

        fputs("      ", stdout);
        loogal_json_string(stdout, "search_similar");
        fputs(": ", stdout);
        loogal_json_string(stdout, search_action);
        fputc('\n', stdout);

        fputs("      }\n", stdout);
        printf("    }%s\n", (i + 1 < end) ? "," : "");
    }

    puts("  ]");
    puts("}");
}

static void print_human_results(
    const char *query_path,
    const char *place_filter,
    double min_percent,
    LoogalSearchEngine engine,
    size_t candidates,
    size_t total_hits,
    Hit *hits,
    size_t offset,
    size_t limit,
    double elapsed_ms
) {
    size_t end = offset + limit;

    if (end > total_hits) end = total_hits;

    size_t returned = offset < total_hits ? end - offset : 0;
    double elapsed_s = elapsed_ms / 1000.0;
    double elapsed_ns = elapsed_s * 1000000000.0;
    double images_per_second = elapsed_s > 0.0 ? ((double)candidates / elapsed_s) : 0.0;
    double images_per_ns = elapsed_ns > 0.0 ? ((double)candidates / elapsed_ns) : 0.0;
    double ns_per_image = candidates > 0 ? (elapsed_ns / (double)candidates) : 0.0;

    puts("Loogal search results:");
    printf("target:     %s\n", query_path);
    printf("scope:      %s\n", (place_filter && *place_filter) ? place_filter : "memory/binary-index");
    printf("engine:     %s\n", engine == LOOGAL_SEARCH_ENGINE_SHADOW ? "binary_index:shadow:v1" : "binary_index:dhash:v1");
    printf("identity:   locations.jsonl:v1\n");
    printf("min:        %.2f%%\n", min_percent);
    printf("candidates: %zu\n", candidates);
    printf("total hits: %zu\n", total_hits);
    printf("returned:   %zu\n", returned);
    printf("time:       %.6f sec / %.0f ns\n", elapsed_s, elapsed_ns);
    printf("speed:      %'.0f images/sec\n", images_per_second);
    printf("nano:       %.12f images/ns\n", images_per_ns);
    printf("cost:       %.0f ns/image\n\n", ns_per_image);

    for (size_t i = offset; i < end; i++) {
        Hit *h = &hits[i];
        const char *label = h->hash_distance == 0 ? " [EXACT]" : "";
        SearchIdentityInfo identity = load_identity_info_for_path(h->rec.path);

        printf("%zu.%s %s\n", i + 1, label, h->rec.path);
        if (engine == LOOGAL_SEARCH_ENGINE_SHADOW) {
            printf(
                "   match: %.3f%%  shadow: %.6f  dhash: %.3f%% (%d/64)  ahash: %.3f%% (%d/64)  aspect: %.3f%%\n",
                h->similarity * 100.0,
                h->similarity,
                h->dhash_similarity * 100.0,
                h->hash_distance,
                h->ahash_similarity * 100.0,
                h->ahash_distance,
                h->aspect_similarity * 100.0
            );
        } else {
            printf(
                "   match: %.3f%%  similarity: %.6f  hash: %d/64\n",
                h->similarity * 100.0,
                h->similarity,
                h->hash_distance
            );
        }

        if (identity.identity_id) {
            printf("   identity_id: %llu  known_locations: %ld\n", identity.identity_id, identity.location_count);

            for (long j = 0; j < identity.stored_locations; j++) {
                printf("      - %s\n", identity.locations[j]);
            }
        } else {
            printf("   identity_id: unknown  known_locations: 0\n");
        }
    }
}

int cmd_search(int argc, char **argv) {
    if (argc < 1) {
        loogal_die("search", "usage: loogal search <image> [place] [--min N] [--limit N] [--offset N] [--json]");
        return 1;
    }

    const char *query_arg = argv[0];
    char query_path[LOOGAL_PATH_MAX];

    normalize_path_best_effort(query_arg, query_path, sizeof(query_path));

    char place_filter[LOOGAL_PATH_MAX] = "";
    double min_percent = 60.0;
    LoogalSearchEngine engine = LOOGAL_SEARCH_ENGINE_DHASH;
    size_t limit = 10;
    size_t offset = 0;
int hash_mode_v0 = 0;
    int as_json = 0;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (strcmp(a, "--json") == 0) {
            as_json = 1;
            continue;
        }

        if (strcmp(a, "--memory") == 0) {
            place_filter[0] = '\0';
            continue;
        }

        if (strcmp(a, "--engine") == 0 && i + 1 < argc) {
            const char *engine_name = argv[++i];

            if (strcmp(engine_name, "shadow") == 0 || strcmp(engine_name, "fused") == 0) {
                engine = LOOGAL_SEARCH_ENGINE_SHADOW;
            } else if (strcmp(engine_name, "dhash") == 0) {
                engine = LOOGAL_SEARCH_ENGINE_DHASH;
            } else {
                loogal_die("search", "unknown engine; use dhash or shadow");
                return 1;
            }

            continue;
        }

        if (strcmp(a, "--shadow") == 0) {
            engine = LOOGAL_SEARCH_ENGINE_SHADOW;
            continue;
        }

        if (strcmp(a, "--min") == 0 && i + 1 < argc) {
            min_percent = atof(argv[++i]);
            continue;
        }

        if (strcmp(a, "--limit") == 0 && i + 1 < argc) {
            long v = atol(argv[++i]);

            if (v > 0) limit = (size_t)v;

            continue;
        }

        if (strcmp(a, "--offset") == 0 && i + 1 < argc) {
            long v = atol(argv[++i]);

            if (v >= 0) offset = (size_t)v;

            continue;
        }

        if (strcmp(a, "--hash-mode") == 0 && i + 1 < argc) {
const char *mode = argv[++i];

if (strcmp(mode, "v0") == 0 || strcmp(mode, "magick-v0") == 0) {
hash_mode_v0 = 1;
} else if (strcmp(mode, "native") == 0) {
hash_mode_v0 = 0;
} else {
loogal_die("search", "unknown --hash-mode value; use native or v0");
return 1;
}

continue;
}

if (is_numberish(a)) {
            min_percent = atof(a);
            continue;
        }

        if (a[0] != '-' && place_filter[0] == '\0') {
            normalize_path_best_effort(a, place_filter, sizeof(place_filter));
            continue;
        }
    }

    if (min_percent < 0.0) min_percent = 0.0;
    if (min_percent > 100.0) min_percent = 100.0;

    if (limit == 0) limit = 10;
    if (limit > 10000) limit = 10000;

    double start_ms = loogal_now_ms();

    LoogalImageInfo query;

    if ((hash_mode_v0 ? image_probe_v0(query_arg, &query) : image_probe(query_arg, &query)) != 0) {
        loogal_die("search", "could not read query image");
        return 1;
    }

    LoogalRecord *records = NULL;
    size_t count = 0;

    if (read_index_records(&records, &count) != 0) {
        loogal_die("search", "could not read binary index");
        return 1;
    }

    Hit *hits = calloc(count ? count : 1, sizeof(Hit));

    if (!hits && count) {
        free(records);
        loogal_die("search", "out of memory allocating hit list");
        return 1;
    }

    size_t candidate_count = 0;
    size_t hcount = 0;

    for (size_t i = 0; i < count; i++) {
        if (place_filter[0] && !starts_with_path(records[i].path, place_filter)) {
            continue;
        }

        candidate_count++;

        int dist = hamming64(query.dhash, records[i].dhash);
        int adist = hamming64(query.ahash, records[i].ahash);

        double dsim = 1.0 - ((double)dist / 64.0);
        double asim = 1.0 - ((double)adist / 64.0);
        double aspect_sim = 0.0;
        double sim = dsim;

        if (query.aspect > 0.0f && records[i].aspect > 0.0f) {
            float qa = query.aspect;
            float ra = records[i].aspect;
            float small = qa < ra ? qa : ra;
            float large = qa > ra ? qa : ra;

            if (large > 0.0f) {
                aspect_sim = (double)(small / large);
            }
        }

        if (engine == LOOGAL_SEARCH_ENGINE_SHADOW) {
            LoogalShadow qs;
            LoogalShadow rs;

            memset(&qs, 0, sizeof(qs));
            memset(&rs, 0, sizeof(rs));

            qs.dhash64 = query.dhash;
            qs.ahash64 = query.ahash;
            qs.width = query.width;
            qs.height = query.height;
            qs.aspect = query.aspect;
            qs.has_dhash64 = 1;
            qs.has_ahash64 = 1;

            rs.dhash64 = records[i].dhash;
            rs.ahash64 = records[i].ahash;
            rs.width = records[i].width;
            rs.height = records[i].height;
            rs.aspect = records[i].aspect;
            rs.has_dhash64 = 1;
            rs.has_ahash64 = 1;

            sim = loogal_shadow_fused_score(&qs, &rs);
        }

        double pct = sim * 100.0;

        if (pct + 0.000001 >= min_percent) {
            hits[hcount].hash_distance = dist;
            hits[hcount].ahash_distance = adist;
            hits[hcount].similarity = sim;
            hits[hcount].dhash_similarity = dsim;
            hits[hcount].ahash_similarity = asim;
            hits[hcount].aspect_similarity = aspect_sim;
            hits[hcount].rec = records[i];
            hcount++;
        }
    }

    qsort(hits, hcount, sizeof(Hit), cmp_hit);

    double elapsed_ms = loogal_now_ms() - start_ms;

    if (as_json) {
        print_json_results(query_path, place_filter, min_percent, engine, candidate_count, hcount, hits, offset, limit, elapsed_ms);
    } else {
        print_human_results(query_path, place_filter, min_percent, engine, candidate_count, hcount, hits, offset, limit, elapsed_ms);
    }

    const char *place_label = place_filter[0] ? place_filter : "memory";
    int msg_len = snprintf(
        NULL,
        0,
        "query=%.180s place=%.180s min=%.3f candidates=%zu hits=%zu limit=%zu offset=%zu json=%d duration_ms=%.3f engine=%s identity_expansion=locations.jsonl:v1",
        query_path,
        place_label,
        min_percent,
        candidate_count,
        hcount,
        limit,
        offset,
        as_json,
        elapsed_ms,
        engine == LOOGAL_SEARCH_ENGINE_SHADOW ? "binary_index:shadow:v1" : "binary_index:dhash:v1"
    );

    if (msg_len > 0) {
        char *msg = malloc((size_t)msg_len + 1);
        if (msg) {
            snprintf(
                msg,
                (size_t)msg_len + 1,
                "query=%.180s place=%.180s min=%.3f candidates=%zu hits=%zu limit=%zu offset=%zu json=%d duration_ms=%.3f engine=%s identity_expansion=locations.jsonl:v1",
                query_path,
                place_label,
                min_percent,
                candidate_count,
                hcount,
                limit,
                offset,
                as_json,
                elapsed_ms,
                engine == LOOGAL_SEARCH_ENGINE_SHADOW ? "binary_index:shadow:v1" : "binary_index:dhash:v1"
            );
            loogal_log("search.complete", "ok", msg);
            free(msg);
        } else {
            loogal_log("search.complete", "ok", "search completed; log message allocation failed");
        }
    } else {
        loogal_log("search.complete", "ok", "search completed");
    }

    free(hits);
    free(records);

    return 0;
}
