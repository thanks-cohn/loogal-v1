#include "loogal.h"
#include "jsonout.h"
#include "region_search.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    LoogalRecord rec;
    int dhash_distance;
    int ahash_distance;
    double score;
} RegionHit;

static int region_cmp_hit(const void *a, const void *b) {
    const RegionHit *ha = (const RegionHit *)a;
    const RegionHit *hb = (const RegionHit *)b;

    if (ha->score < hb->score) return 1;
    if (ha->score > hb->score) return -1;

    if (ha->dhash_distance != hb->dhash_distance) {
        return ha->dhash_distance - hb->dhash_distance;
    }

    return strcmp(ha->rec.path, hb->rec.path);
}

static int has_arg(int argc, char **argv, const char *needle) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], needle) == 0) return 1;
    }

    return 0;
}

static int path_has_scope(const char *path, const char *scope) {
    if (!scope || !*scope || strcmp(scope, "memory") == 0 || strcmp(scope, "--memory") == 0) {
        return 1;
    }

    size_t n = strlen(scope);

    if (strncmp(path, scope, n) != 0) return 0;

    return path[n] == '\0' || path[n] == '/' || path[n] == '\\';
}

static double hash_similarity(int distance) {
    if (distance < 0) return 0.0;
    if (distance > 64) return 0.0;

    return 1.0 - ((double)distance / 64.0);
}

static double region_score(const LoogalImageInfo *query, const LoogalRecord *rec, int dd, int ad) {
    double dsim = hash_similarity(dd);
    double asim = hash_similarity(ad);
    double aspect = 0.0;
    double size_fit = 0.0;

    if (query->aspect > 0.0f && rec->aspect > 0.0f) {
        double qa = query->aspect;
        double ra = rec->aspect;
        double small = qa < ra ? qa : ra;
        double large = qa > ra ? qa : ra;

        if (large > 0.0) aspect = small / large;
    }

    if (query->width > 0 && query->height > 0 && rec->width > 0 && rec->height > 0) {
        if (query->width <= rec->width && query->height <= rec->height) {
            size_fit = 1.0;
        } else {
            double qw = (double)query->width;
            double qh = (double)query->height;
            double rw = (double)rec->width;
            double rh = (double)rec->height;
            double wr = rw > 0.0 ? qw / rw : 0.0;
            double hr = rh > 0.0 ? qh / rh : 0.0;
            double penalty = wr > hr ? wr : hr;

            size_fit = penalty > 0.0 ? 1.0 / penalty : 0.0;
        }
    }

    return (dsim * 0.55) + (asim * 0.25) + (aspect * 0.10) + (size_fit * 0.10);
}

static void print_region_json(
    const char *query,
    const char *scope,
    RegionHit *hits,
    size_t count,
    size_t limit,
    double elapsed_ms
) {
    size_t returned = count < limit ? count : limit;

    puts("{");
    printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
    printf("  "); loogal_json_kv_string(stdout, "type", "region.search.results", 1);
    printf("  "); loogal_json_kv_string(stdout, "engine", "shardscan:v1:whole-record-candidates", 1);
    printf("  "); loogal_json_kv_string(stdout, "query", query, 1);
    printf("  "); loogal_json_kv_string(stdout, "scope", scope, 1);
    printf("  \"total_hits\": %zu,\n", count);
    printf("  \"returned\": %zu,\n", returned);
    printf("  \"duration_ms\": %.3f,\n", elapsed_ms);
    puts("  \"results\": [");

    for (size_t i = 0; i < returned; i++) {
        RegionHit *h = &hits[i];

        printf("    {\n");
        printf("      \"rank\": %zu,\n", i + 1);
        printf("      \"path\": "); loogal_json_string(stdout, h->rec.path); puts(",");
        printf("      \"score\": %.6f,\n", h->score);
        printf("      \"score_percent\": %.3f,\n", h->score * 100.0);
        printf("      \"dhash_distance\": %d,\n", h->dhash_distance);
        printf("      \"ahash_distance\": %d,\n", h->ahash_distance);
        printf("      \"candidate_region\": {\n");
        printf("        \"x\": 0,\n");
        printf("        \"y\": 0,\n");
        printf("        \"width\": %d,\n", h->rec.width);
        printf("        \"height\": %d\n", h->rec.height);
        printf("      },\n");
        printf("      \"actions\": {\n");
        printf("        \"land\": ");
        printf("\"loogal action reveal --path '");
        for (const char *p = h->rec.path; *p; p++) {
            if (*p == '"' || *p == '\\') putchar('\\');
            putchar(*p);
        }
        printf("'\"\n");
        printf("      }\n");
        printf("    }%s\n", (i + 1 < returned) ? "," : "");
    }

    puts("  ]");
    puts("}");
}

int cmd_region_search(int argc, char **argv) {
    if (argc < 1) {
        loogal_die(
            "region-search",
            "usage: loogal region-search <crop.png> [place] [--min N] [--limit N] [--json]"
        );

        return 1;
    }

    const char *query_path = argv[0];
    const char *scope = "memory";
    double min_percent = 60.0;
    size_t limit = 10;
    int as_json = has_arg(argc, argv, "--json");

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--json") == 0) continue;
        if (strcmp(argv[i], "--memory") == 0) {
            scope = "memory";
            continue;
        }
        if (strcmp(argv[i], "--min") == 0 && i + 1 < argc) {
            min_percent = atof(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            long v = atol(argv[++i]);
            if (v > 0) limit = (size_t)v;
            continue;
        }
        if (argv[i][0] != '-') {
            scope = argv[i];
        }
    }

    if (min_percent < 0.0) min_percent = 0.0;
    if (min_percent > 100.0) min_percent = 100.0;
    if (limit == 0) limit = 10;
    if (limit > 1000) limit = 1000;

    double start_ms = loogal_now_ms();

    LoogalImageInfo query;
    if (image_probe(query_path, &query) != 0) {
        loogal_die("region-search", "could not read query image");
        return 1;
    }

    LoogalRecord *records = NULL;
    size_t record_count = 0;

    if (read_index_records(&records, &record_count) != 0) {
        loogal_die("region-search", "could not read binary index");
        return 1;
    }

    RegionHit *hits = calloc(record_count ? record_count : 1, sizeof(RegionHit));
    if (!hits && record_count) {
        free(records);
        loogal_die("region-search", "out of memory allocating hits");
        return 1;
    }

    size_t hit_count = 0;

    for (size_t i = 0; i < record_count; i++) {
        if (!path_has_scope(records[i].path, scope)) continue;

        int dd = hamming64(query.dhash, records[i].dhash);
        int ad = hamming64(query.ahash, records[i].ahash);
        double score = region_score(&query, &records[i], dd, ad);

        if ((score * 100.0) + 0.000001 < min_percent) continue;

        hits[hit_count].rec = records[i];
        hits[hit_count].dhash_distance = dd;
        hits[hit_count].ahash_distance = ad;
        hits[hit_count].score = score;
        hit_count++;
    }

    qsort(hits, hit_count, sizeof(RegionHit), region_cmp_hit);

    double elapsed_ms = loogal_now_ms() - start_ms;

    if (as_json) {
        print_region_json(query_path, scope, hits, hit_count, limit, elapsed_ms);
    } else {
        size_t returned = hit_count < limit ? hit_count : limit;

        puts("REGION SEARCH");
        puts("==============================");
        printf("query: %s\n", query_path);
        printf("scope: %s\n", scope);
        puts("engine: shardscan:v1:whole-record-candidates");
        printf("hits: %zu\n", hit_count);
        printf("time: %.3f ms\n", elapsed_ms);
        puts("");

        for (size_t i = 0; i < returned; i++) {
            printf("%zu. %.3f%%  %s\n", i + 1, hits[i].score * 100.0, hits[i].rec.path);
            printf("   candidate_region: x=0 y=0 w=%d h=%d\n", hits[i].rec.width, hits[i].rec.height);
            printf("   dhash=%d/64 ahash=%d/64\n", hits[i].dhash_distance, hits[i].ahash_distance);
        }
    }

    loogal_log("region-search.complete", "ok", query_path);

    free(hits);
    free(records);
    return 0;
}
