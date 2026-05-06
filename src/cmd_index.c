#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    LoogalMemory memory;
    size_t scanned_files;
    size_t skipped_files;
    int fresh;
    int dry_run;
} LoogalIndexContext;

static LoogalIndexContext *g_ctx = NULL;

/*
 * records.jsonl is an active compatibility projection.
 *
 * It is intentionally rebuilt from durable memory on each successful index run.
 * It is not the sacred append-only truth layer.
 *
 * Durable memory lives in:
 *   - identities.jsonl
 *   - locations.jsonl
 *   - events.jsonl
 *
 * This keeps indexing boring and dependable:
 * observations/events are additive,
 * identities and locations are compacted durable state,
 * records.jsonl and loogal.bin are rebuildable speed/compatibility outputs.
 */
static void write_compat_records_jsonl(const LoogalRecord *records, size_t count) {
    FILE *f = fopen(LOOGAL_RECORDS_PATH, "w");
    if (!f) {
        loogal_log("records.compat", "warn", "could not write records.jsonl active compatibility projection");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        const LoogalRecord *r = &records[i];
        char *p = json_escape(r->path);
        fprintf(f, "{\"id\":%llu,\"tool\":\"loogal\",\"type\":\"image_record\",\"path\":\"%s\",\"width\":%d,\"height\":%d,\"file_size_bytes\":%llu,\"dhash\":\"%016llx\",\"ahash\":\"%016llx\",\"signature_engine\":\"dhash:v1+ahash:v1\",\"projection\":\"active_memory:v1\"}\n",
                (unsigned long long)r->id,
                p ? p : "",
                r->width,
                r->height,
                (unsigned long long)r->file_size,
                (unsigned long long)r->dhash,
                (unsigned long long)r->ahash);
        free(p);
    }

    fclose(f);
}

static int parse_flags(int argc, char **argv, int *fresh, int *dry_run, int *first_path) {
    *fresh = 0;
    *dry_run = 0;
    *first_path = 0;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--fresh") == 0) {
            *fresh = 1;
        } else if (strcmp(argv[i], "--dry-run") == 0) {
            *dry_run = 1;
        } else if (argv[i][0] == '-') {
            char msg[256];
            snprintf(msg, sizeof(msg), "unknown index flag: %s", argv[i]);
            loogal_die("index.flags", msg);
            return -1;
        } else {
            *first_path = i;
            return 0;
        }
    }
    return 0;
}

static int visitor(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    (void)sb;
    (void)ftwbuf;

    if (typeflag != FTW_F) return 0;
    if (!image_is_supported(fpath)) return 0;

    g_ctx->scanned_files++;

    LoogalImageInfo info;
    if (image_probe(fpath, &info) == 0) {
        if (!g_ctx->dry_run) {
            loogal_memory_ingest_image(&g_ctx->memory, &info);
        }
        if (g_ctx->scanned_files % 100 == 0) {
            char msg[160];
            snprintf(msg, sizeof(msg), "scanned=%zu new_identities=%zu new_locations=%zu known=%zu",
                     g_ctx->scanned_files,
                     g_ctx->memory.seen_new_identities,
                     g_ctx->memory.seen_new_locations,
                     g_ctx->memory.seen_known_locations);
            loogal_log("index.progress", "ok", msg);
        }
    } else {
        g_ctx->skipped_files++;
        loogal_log("index.skip", "warn", fpath);
    }

    return 0;
}

int cmd_index(int argc, char **argv) {
    if (argc < 1) {
        loogal_die("index", "usage: loogal index [--fresh] [--dry-run] <directories...>");
        return 1;
    }

    ensure_dirs();

    static LoogalIndexContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    g_ctx = &ctx;

    int first_path = 0;
    if (parse_flags(argc, argv, &ctx.fresh, &ctx.dry_run, &first_path) != 0) return 1;
    if (first_path >= argc) {
        loogal_die("index", "usage: loogal index [--fresh] [--dry-run] <directories...>");
        return 1;
    }

    if (ctx.fresh && !ctx.dry_run) {
        loogal_memory_reset_files();
        loogal_log("index.fresh", "ok", "fresh index requested; old memory files removed");
    }

    if (loogal_memory_load(&ctx.memory) != 0) {
        loogal_die("index", "could not load memory files");
        return 1;
    }

    loogal_log("index.start", "ok", ctx.dry_run ? "starting non-destructive dry-run" : "starting bedrock memory index: durable memory merge plus rebuildable projections");

    for (int i = first_path; i < argc; i++) {
        char msg[LOOGAL_PATH_MAX + 96];
        snprintf(msg, sizeof(msg), "scanning %s", argv[i]);
        loogal_log("index.scan_dir", "ok", msg);
        if (nftw(argv[i], visitor, 32, FTW_PHYS) != 0) {
            loogal_log("index.scan_dir", "warn", argv[i]);
        }
    }

    LoogalRecord *records = NULL;
    size_t record_count = 0;

    if (loogal_memory_build_records(&ctx.memory, &records, &record_count) != 0) {
        loogal_memory_free(&ctx.memory);
        loogal_die("index", "failed building active memory projection");
        return 1;
    }

    if (!ctx.dry_run) {
        if (loogal_memory_save(&ctx.memory) != 0) {
            free(records);
            loogal_memory_free(&ctx.memory);
            loogal_die("index", "failed saving memory files");
            return 1;
        }
        write_compat_records_jsonl(records, record_count);
        if (write_index_records(records, record_count) != 0) {
            free(records);
            loogal_memory_free(&ctx.memory);
            loogal_die("index", "failed writing binary index");
            return 1;
        }
    }

    printf("LOOGAL INDEX COMPLETE\n");
    printf("  Mode              : %s\n", ctx.dry_run ? "dry-run" : (ctx.fresh ? "fresh rebuild" : "bedrock merge / non-destructive"));
    printf("  Records           : %s active projection, rebuilt from durable memory\n", LOOGAL_RECORDS_PATH);
    printf("  Files scanned     : %zu\n", ctx.scanned_files);
    printf("  Files skipped     : %zu\n", ctx.skipped_files);
    printf("  New identities    : %zu\n", ctx.memory.seen_new_identities);
    printf("  New locations     : %zu\n", ctx.memory.seen_new_locations);
    printf("  Known locations   : %zu\n", ctx.memory.seen_known_locations);
    printf("  Duplicate refs    : %zu\n", ctx.memory.seen_duplicate_locations);
    printf("  Modified paths    : %zu\n", ctx.memory.seen_modified_paths);
    printf("  Active records    : %zu\n", record_count);
    printf("  Binary index      : %s\n", LOOGAL_BIN_PATH);
    printf("  Identities        : %s\n", LOOGAL_IDENTITIES_PATH);
    printf("  Locations         : %s\n", LOOGAL_LOCATIONS_PATH);
    printf("  Events            : %s\n", LOOGAL_EVENTS_PATH);
    printf("  Logs              : %s\n", LOOGAL_LOG_PATH);

    char msg[256];
    snprintf(msg, sizeof(msg), "scanned=%zu active_records=%zu new_identities=%zu new_locations=%zu known=%zu duplicates=%zu modified=%zu",
             ctx.scanned_files, record_count,
             ctx.memory.seen_new_identities,
             ctx.memory.seen_new_locations,
             ctx.memory.seen_known_locations,
             ctx.memory.seen_duplicate_locations,
             ctx.memory.seen_modified_paths);
    loogal_log("index.complete", "ok", msg);
    loogal_memory_append_event("index.complete", "ok", "", 0, msg);

    free(records);
    loogal_memory_free(&ctx.memory);
    g_ctx = NULL;
    return 0;
}
