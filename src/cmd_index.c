#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>

typedef struct {
    LoogalMemory memory;
    size_t scanned_files;
    size_t skipped_files;
size_t known_unchanged_skipped;
    int fresh;
    int dry_run;
int hash_mode_v0;
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

static int parse_flags(int argc, char **argv, int *fresh, int *dry_run, int *hash_mode_v0, int *first_path) {
*fresh = 0;
*dry_run = 0;
*hash_mode_v0 = 0;
*first_path = 0;

for (int i = 0; i < argc; i++) {
if (strcmp(argv[i], "--fresh") == 0) {
*fresh = 1;
} else if (strcmp(argv[i], "--dry-run") == 0) {
*dry_run = 1;
} else if (strcmp(argv[i], "--hash-mode") == 0) {
if (i + 1 >= argc) {
loogal_die("index.flags", "missing value after --hash-mode");
return -1;
}

i++;

if (strcmp(argv[i], "v0") == 0 || strcmp(argv[i], "magick-v0") == 0) {
*hash_mode_v0 = 1;
} else if (strcmp(argv[i], "native") == 0) {
*hash_mode_v0 = 0;
} else {
loogal_die("index.flags", "unknown --hash-mode value; use native or v0");
return -1;
}
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

static int index_one_file(const char *fpath) {
    struct stat st;

    printf("[walk] file: %s\n", fpath);

    if (!image_is_supported(fpath)) {
        printf("[skip] unsupported: %s\n", fpath);
        return 0;
    }

    if (stat(fpath, &st) != 0) {
        g_ctx->skipped_files++;
        printf("[skip] stat failed: %s\n", fpath);
        return 0;
    }

    g_ctx->scanned_files++;

    if (!g_ctx->fresh) {
        uint64_t file_size = (uint64_t)st.st_size;
        uint64_t mtime_unix = (uint64_t)st.st_mtime;

        if (loogal_memory_mark_known_location_if_unchanged(
                &g_ctx->memory,
                fpath,
                file_size,
                mtime_unix,
                1
            )) {
            g_ctx->known_unchanged_skipped++;
            return 0;
        }
    }

    LoogalImageInfo info;

    if ((g_ctx->hash_mode_v0 ? image_probe_v0(fpath, &info) : image_probe(fpath, &info)) == 0) {
        info.file_size = (uint64_t)st.st_size;
        info.mtime_unix = (uint64_t)st.st_mtime;

        if (!g_ctx->dry_run) {
            loogal_memory_ingest_image(&g_ctx->memory, &info);
        }

        printf("[image] indexed: %s\n", fpath);
    } else {
        g_ctx->skipped_files++;
        loogal_log("index.skip", "warn", fpath);
        printf("[skip] image probe failed: %s\n", fpath);
    }

    return 0;
}

static int walk_dir_win32(const char *root) {
    char pattern[LOOGAL_PATH_MAX];
    WIN32_FIND_DATAA fd;
    HANDLE h;

    snprintf(pattern, sizeof(pattern), "%s\\*", root);

    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        printf("[walk-warn] cannot open: %s\n", root);
        return -1;
    }

    do {
        char path[LOOGAL_PATH_MAX];

        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s\\%s", root, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            walk_dir_win32(path);
        } else {
            index_one_file(path);
        }
    } while (FindNextFileA(h, &fd));

    FindClose(h);
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
    if (parse_flags(argc, argv, &ctx.fresh, &ctx.dry_run, &ctx.hash_mode_v0, &first_path) != 0) return 1;
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
        if (walk_dir_win32(argv[i]) != 0) {
            loogal_log("index.scan_dir", "warn", argv[i]);
        }
    }

int changed = ctx.fresh ||
ctx.memory.seen_new_identities > 0 ||
ctx.memory.seen_new_locations > 0 ||
ctx.memory.seen_duplicate_locations > 0 ||
ctx.memory.seen_modified_paths > 0;

LoogalRecord *records = NULL;
size_t record_count = ctx.memory.location_count;

if (changed || ctx.dry_run) {
if (loogal_memory_build_records(&ctx.memory, &records, &record_count) != 0) {
loogal_memory_free(&ctx.memory);
loogal_die("index", "failed building active memory projection");
return 1;
}
}

if (!ctx.dry_run && changed) {
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
printf("  Known unchanged skipped: %zu\n", ctx.known_unchanged_skipped);
printf("  Projection update : %s\n", changed ? "rebuilt" : "skipped unchanged");
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

