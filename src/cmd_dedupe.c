#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "loogal/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "pathsafe.h"
#include "receipt.h"
#include "manifest.h"
#include "protect.h"

static int better(const LoogalRecord *a, const LoogalRecord *b) {
    long long pixels_a = (long long)a->width * a->height;
    long long pixels_b = (long long)b->width * b->height;
    if (pixels_a != pixels_b) return pixels_a > pixels_b;
    if (a->file_size != b->file_size) return a->file_size > b->file_size;
    return strlen(a->path) < strlen(b->path);
}

static void mkdir_p_simple(const char *dir) {
    char tmp[LOOGAL_PATH_MAX]; snprintf(tmp, sizeof(tmp), "%s", dir);
    for (char *p = tmp + 1; *p; p++) if (*p == '/') { *p = 0; loogal_platform_mkdir(tmp); *p = '/'; }
    loogal_platform_mkdir(tmp);
}

static int basename_copy(const char *path, char *out, size_t n) {
    const char *s = strrchr(path, '/');
    s = s ? s + 1 : path;
    return loogal_copy_path(out, n, s);
}


static int safe_move(const LoogalRecord *move, const LoogalRecord *keep, const char *dest_dir, int pct, FILE *manifest) {
    mkdir_p_simple(dest_dir);
    char base[512];
if (basename_copy(move->path, base, sizeof(base)) != 0) {
    loogal_log("dedupe", "error", "path basename too long");
    return -1;
}
    char dest[LOOGAL_PATH_MAX]; snprintf(dest, sizeof(dest), "%s/%s", dest_dir, base);
    int suffix = 1;
    while (loogal_platform_path_exists(dest)) {
        snprintf(dest, sizeof(dest), "%s/%d-%s", dest_dir, suffix++, base);
    }
    if (rename(move->path, dest) != 0) return -1;
    if (loogal_write_move_receipt(move, keep, dest, pct) != 0) {
loogal_log("dedupe", "error", "failed to write move receipt");
return -1;
}
if (loogal_manifest_write_move(manifest, move, keep, dest, base, pct) != 0) {
loogal_log("dedupe", "error", "failed to write manifest move");
return -1;
}
return 0;
}

int cmd_dedupe(int argc, char **argv) {
    int keep_n = 1, dry = 0; const char *move_dir = NULL;
    char *protects[64]; int protect_n = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--keep") == 0 && i + 1 < argc) keep_n = atoi(argv[++i]);
        else if (strcmp(argv[i], "--dry-run") == 0) dry = 1;
        else if (strcmp(argv[i], "--move-removed") == 0 && i + 1 < argc) move_dir = argv[++i];
        else if (strcmp(argv[i], "--protect") == 0) {
            while (i + 1 < argc && strncmp(argv[i+1], "--", 2) != 0 && protect_n < 64) protects[protect_n++] = argv[++i];
        }
    }
    if (keep_n < 1) keep_n = 1;
    if (!dry && !move_dir) { loogal_die("dedupe", "apply mode requires --move-removed DIR, or use --dry-run"); return 1; }
    LoogalRecord *records = NULL; size_t count = 0;
    if (read_index_records(&records, &count) != 0) return 1;
    char ts[64]; iso_time_now(ts, sizeof(ts));
    char manifest_path[LOOGAL_PATH_MAX]; snprintf(manifest_path, sizeof(manifest_path), "%s/loogal_dedupe_%ld.json", LOOGAL_MANIFEST_DIR, now_unix());
    FILE *manifest = fopen(manifest_path, "w");
    if (!manifest) { free(records); loogal_die("dedupe", "could not write manifest"); return 1; }
    fprintf(manifest, "{\n  \"tool\":\"loogal\",\n  \"event\":\"dedupe\",\n  \"dry_run\":%s,\n  \"keep\":%d,\n  \"moved\":[\n", dry ? "true" : "false", keep_n);
    int *used = calloc(count, sizeof(int));
    int moved = 0, clusters = 0, first_manifest = 1;
    puts("LOOGAL DEDUPE");
    for (size_t i = 0; i < count; i++) {
        if (used[i]) continue;
        size_t members[512]; int m = 0;
        members[m++] = i;
        for (size_t j = i + 1; j < count && m < 512; j++) {
            if (!used[j] && similarity_percent(records[i].dhash, records[j].dhash) >= 90) members[m++] = j;
        }
        if (m <= keep_n) continue;
        clusters++;
        for (int a = 0; a < m; a++) for (int b = a + 1; b < m; b++) {
            int pa = loogal_is_protected_path(records[members[a]].path, protects, protect_n);
            int pb = loogal_is_protected_path(records[members[b]].path, protects, protect_n);
            if ((pb && !pa) || (pa == pb && better(&records[members[b]], &records[members[a]]))) {
                size_t tmp = members[a]; members[a] = members[b]; members[b] = tmp;
            }
        }
        int all_protected = 1;
        for (int k = 0; k < m; k++) if (!loogal_is_protected_path(records[members[k]].path, protects, protect_n)) all_protected = 0;
        if (all_protected) { printf("SKIP protected cluster (%d files)\n", m); continue; }
        printf("Cluster %d files; keep: %s\n", m, records[members[0]].path);
        int kept = 0;
        for (int k = 0; k < m; k++) {
            LoogalRecord *r = &records[members[k]];
            int prot = loogal_is_protected_path(r->path, protects, protect_n);
            if (prot || kept < keep_n) { printf("  KEEP %s%s\n", r->path, prot ? " [protected]" : ""); kept++; continue; }
            int pct = similarity_percent(records[members[0]].dhash, r->dhash);
            printf("  %s %s (%d%%)\n", dry ? "WOULD MOVE" : "MOVE", r->path, pct);
            if (!dry) {
                if (!first_manifest) fprintf(manifest, ",\n");
                if (safe_move(r, &records[members[0]], move_dir, pct, manifest) == 0) { moved++; first_manifest = 0; }
            }
        }
        for (int k = 0; k < m; k++) used[members[k]] = 1;
    }
    fprintf(manifest, "\n  ],\n  \"clusters_seen\":%d,\n  \"files_moved\":%d\n}\n", clusters, moved);
    fclose(manifest);
    printf("\nManifest: %s\n", manifest_path);
    char msg[256]; snprintf(msg, sizeof(msg), "clusters=%d moved=%d dry_run=%d", clusters, moved, dry);
    loogal_log("dedupe.complete", "ok", msg);
    free(used); free(records);
    return 0;
}
