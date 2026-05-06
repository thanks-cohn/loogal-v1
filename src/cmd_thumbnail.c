#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "thumbnail.h"
#include "loogal/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

#define THUMB_DIR "data/thumbnails"

static void usage(void) {
    puts("LOOGAL THUMBNAIL — cached preview images for loogal-window");
    puts("");
    puts("Commands:");
    puts("  loogal thumbnail path --path <image> [--size N] [--json]");
    puts("  loogal thumbnail create --path <image> [--size N] [--force] [--dry-run] [--json]");
    puts("  loogal thumbnail session --session <id> [--offset N] [--limit N] [--size N] [--dry-run] [--json]");
    puts("  loogal thumbnail status [--json]");
    puts("");
    puts("Purpose:");
    puts("  Create small cached previews so the future GTK window does not load full-size images while scrolling.");
}

static int has_flag(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc; i++) if (strcmp(argv[i], flag) == 0) return 1;
    return 0;
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i + 1 < argc; i++) if (strcmp(argv[i], flag) == 0) return argv[i+1];
    return NULL;
}

static int arg_int(int argc, char **argv, const char *flag, int def) {
    const char *v = arg_value(argc, argv, flag);
    if (!v) return def;
    int n = atoi(v);
    return n > 0 ? n : def;
}

static int mkdir_if_needed(const char *p) {
    if (mkdir(p, 0755) != 0 && errno != EEXIST) return -1;
    return 0;
}


static uint64_t fnv1a64_bytes(const unsigned char *s, size_t n, uint64_t h) {
    for (size_t i = 0; i < n; i++) {
        h ^= (uint64_t)s[i];
        h *= 1099511628211ULL;
    }
    return h;
}


static int loogal_count_thumbnail_cache(const char *dir_path, int *out_count, uint64_t *out_bytes) {
    if (!dir_path || !out_count || !out_bytes) return -1;

    *out_count = 0;
    *out_bytes = 0;

    DIR *dir = opendir(dir_path);
    if (!dir) {
        return 0;
    }

    for (;;) {
        errno = 0;
        struct dirent *ent = readdir(dir);

        if (!ent) {
            if (errno != 0) {
                closedir(dir);
                return -1;
            }
            break;
        }

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char path[LOOGAL_PATH_MAX * 2];
        int n = snprintf(path, sizeof(path), "%s/%s", dir_path, ent->d_name);

        if (n < 0 || n >= (int)sizeof(path)) {
            continue;
        }

        struct stat st;

        if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
            (*out_count)++;
            if (st.st_size > 0) {
                *out_bytes += (uint64_t)st.st_size;
            }
        }
    }

    closedir(dir);
    return 0;
}

static uint64_t loogal_thumb_key(const char *path, int size) {
    struct stat st;
    uint64_t h = 1469598103934665603ULL;
    h = fnv1a64_bytes((const unsigned char *)path, strlen(path), h);
    if (stat(path, &st) == 0) {
        h = fnv1a64_bytes((const unsigned char *)&st.st_size, sizeof(st.st_size), h);
        h = fnv1a64_bytes((const unsigned char *)&st.st_mtime, sizeof(st.st_mtime), h);
    }
    h = fnv1a64_bytes((const unsigned char *)&size, sizeof(size), h);
    return h;
}

int loogal_thumbnail_cache_path(const char *image_path, int size, char *out, size_t out_sz) {
    if (!image_path || !out || out_sz == 0) return -1;
    if (size <= 0) size = 160;
    uint64_t key = loogal_thumb_key(image_path, size);
    int n = snprintf(out, out_sz, "%s/%016llx_%d.jpg", THUMB_DIR, (unsigned long long)key, size);
    if (n < 0 || n >= (int)out_sz) return -1;
    return 0;
}

static int json_str(FILE *f, const char *key, const char *val, int comma) {
    char *e = json_escape(val ? val : "");
    if (!e) return -1;
    fprintf(f, "\"%s\": \"%s\"%s\n", key, e, comma ? "," : "");
    free(e);
    return 0;
}

static int create_one(const char *path, int size, int force, int dry_run, int as_json, char *thumb_out, size_t thumb_out_sz) {
    if (!path || !*path) {
        fprintf(stderr, "[loogal:thumbnail_error] missing --path\n");
        loogal_log("thumbnail", "error", "missing path");
        return 1;
    }
    if (!image_is_supported(path)) {
        fprintf(stderr, "[loogal:thumbnail_error] unsupported image type: %s\n", path);
        loogal_log("thumbnail", "error", "unsupported image type");
        return 1;
    }
    if (access(path, R_OK) != 0) {
        fprintf(stderr, "[loogal:thumbnail_error] cannot read image: %s\n", path);
        loogal_log("thumbnail", "error", "cannot read image");
        return 1;
    }
    if (mkdir_if_needed("data") != 0 || mkdir_if_needed(THUMB_DIR) != 0) {
        fprintf(stderr, "[loogal:thumbnail_error] cannot create thumbnail cache directory\n");
        loogal_log("thumbnail", "error", "cannot create thumbnail cache directory");
        return 1;
    }

    char thumb[LOOGAL_PATH_MAX * 2];
    if (loogal_thumbnail_cache_path(path, size, thumb, sizeof(thumb)) != 0) {
        fprintf(stderr, "[loogal:thumbnail_error] thumbnail path too long\n");
        loogal_log("thumbnail", "error", "thumbnail path too long");
        return 1;
    }
    if (thumb_out && thumb_out_sz) snprintf(thumb_out, thumb_out_sz, "%s", thumb);

    int exists = access(thumb, R_OK) == 0;
    int will_create = force || !exists;

    if (!dry_run && will_create) {
    /*
        Zero-dependency fallback:
        We no longer shell out to external image tooling here.

        For now, thumbnail cache creation copies the source image bytes into
        the thumbnail cache path. This preserves GUI/cache plumbing without
        adding runtime dependencies. A future pure-C thumbnail resizer can
        replace this helper without changing the command contract.
    */
    if (loogal_platform_copy_file(path, thumb) != 0) {
        fprintf(stderr, "[loogal:thumbnail_error] native thumbnail fallback failed\n");
        loogal_log("thumbnail", "error", "native thumbnail fallback failed");
        return 1;
    }
}

    if (as_json) {
        puts("{");
        printf("\"tool\": \"loogal\",\n");
        printf("\"type\": \"thumbnail.result\",\n");
        json_str(stdout, "source", path, 1);
        json_str(stdout, "thumbnail", thumb, 1);
        printf("\"size\": %d,\n", size);
        printf("\"cached\": %s,\n", exists && !force ? "true" : "false");
        printf("\"created\": %s,\n", (!dry_run && will_create) ? "true" : "false");
        printf("\"dry_run\": %s\n", dry_run ? "true" : "false");
        puts("}");
    } else {
        printf("[loogal:thumbnail_ok] %s -> %s%s\n", path, thumb, dry_run ? " [dry-run]" : "");
    }

    char msg[1024];
    int n_msg = snprintf(msg, sizeof(msg), "source=%s thumbnail=%s dry_run=%d", path, thumb, dry_run);
    if (n_msg < 0 || n_msg >= (int)sizeof(msg)) {
        snprintf(msg, sizeof(msg), "thumbnail created; message truncated dry_run=%d", dry_run);
    }
    loogal_log("thumbnail", "ok", msg);
    return 0;
}

static int cmd_thumb_path(int argc, char **argv) {
    int as_json = has_flag(argc, argv, "--json");
    int size = arg_int(argc, argv, "--size", 160);
    const char *path = arg_value(argc, argv, "--path");
    if (!path) { fprintf(stderr, "[loogal:thumbnail_error] missing --path\n"); return 1; }
    char thumb[LOOGAL_PATH_MAX * 2];
    if (loogal_thumbnail_cache_path(path, size, thumb, sizeof(thumb)) != 0) {
        fprintf(stderr, "[loogal:thumbnail_error] thumbnail path too long\n");
        return 1;
    }
    if (as_json) {
        puts("{");
        printf("\"tool\": \"loogal\",\n");
        printf("\"type\": \"thumbnail.path\",\n");
        json_str(stdout, "source", path, 1);
        json_str(stdout, "thumbnail", thumb, 1);
        printf("\"size\": %d\n", size);
        puts("}");
    } else {
        puts(thumb);
    }
    return 0;
}

static int extract_json_path(const char *line, char *out, size_t out_sz) {
    const char *p = strstr(line, "\"path\"");
    if (!p) return 0;
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '\"') return 0;
    p++;
    size_t j = 0;
    while (*p && *p != '\"' && j + 1 < out_sz) {
        if (*p == '\\' && p[1]) p++;
        out[j++] = *p++;
    }
    out[j] = 0;
    return j > 0;
}

static int cmd_thumb_session(int argc, char **argv) {
    int as_json = has_flag(argc, argv, "--json");
    int dry_run = has_flag(argc, argv, "--dry-run");
    int force = has_flag(argc, argv, "--force");
    int size = arg_int(argc, argv, "--size", 160);
    int offset = arg_int(argc, argv, "--offset", 0);
    int limit = arg_int(argc, argv, "--limit", 10);
    const char *session = arg_value(argc, argv, "--session");
    if (!session) { fprintf(stderr, "[loogal:thumbnail_error] missing --session\n"); return 1; }

    char results_path[LOOGAL_PATH_MAX * 2];
    int n = snprintf(results_path, sizeof(results_path), "data/sessions/%s/results.json", session);
    if (n < 0 || n >= (int)sizeof(results_path)) {
        fprintf(stderr, "[loogal:thumbnail_error] session path too long\n");
        return 1;
    }
    FILE *f = fopen(results_path, "r");
    if (!f) {
        fprintf(stderr, "[loogal:thumbnail_error] cannot open session results: %s\n", results_path);
        return 1;
    }

    if (as_json) {
        puts("{");
        printf("\"tool\": \"loogal\",\n");
        printf("\"type\": \"thumbnail.session\",\n");
        json_str(stdout, "session", session, 1);
        printf("\"offset\": %d,\n", offset);
        printf("\"limit\": %d,\n", limit);
        printf("\"size\": %d,\n", size);
        printf("\"dry_run\": %s,\n", dry_run ? "true" : "false");
        printf("\"items\": [\n");
    }

    char line[8192];
    int seen = 0, emitted = 0, rc_final = 0;
    while (fgets(line, sizeof(line), f)) {
        char path[LOOGAL_PATH_MAX * 2];
        if (!extract_json_path(line, path, sizeof(path))) continue;
        if (seen++ < offset) continue;
        if (emitted >= limit) break;

        char thumb[LOOGAL_PATH_MAX * 2] = {0};
        int rc = create_one(path, size, force, dry_run, 0, thumb, sizeof(thumb));
        if (rc != 0) rc_final = rc;
        if (as_json) {
            printf("%s{", emitted ? ",\n" : "");
            char *ep = json_escape(path); char *et = json_escape(thumb);
            printf("\"source\": \"%s\", \"thumbnail\": \"%s\", \"status\": \"%s\"}", ep ? ep : "", et ? et : "", rc == 0 ? "ok" : "error");
            free(ep); free(et);
        }
        emitted++;
    }
    fclose(f);

    if (as_json) {
        printf("\n],\n\"returned\": %d\n", emitted);
        puts("}");
    } else {
        printf("[loogal:thumbnail_session_ok] session=%s returned=%d\n", session, emitted);
    }
    return rc_final;
}

static int cmd_thumb_status(int argc, char **argv) {
    int as_json = has_flag(argc, argv, "--json");

    int count = 0;
    uint64_t bytes = 0;

    if (loogal_count_thumbnail_cache(THUMB_DIR, &count, &bytes) != 0) {
        count = 0;
        bytes = 0;
    }

    if (as_json) {
        printf("{\n\"tool\": \"loogal\",\n\"type\": \"thumbnail.status\",\n\"directory\": \"%s\",\n\"count\": %d,\n\"bytes\": %llu\n}\n",
            THUMB_DIR,
            count,
            (unsigned long long)bytes);
    } else {
        printf("LOOGAL THUMBNAILS\n  directory: %s\n  count:     %d\n  bytes:     %llu\n",
            THUMB_DIR,
            count,
            (unsigned long long)bytes);
    }

    return 0;
}

int cmd_thumbnail(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) { usage(); return 0; }
    if (strcmp(argv[0], "path") == 0) return cmd_thumb_path(argc - 1, argv + 1);
    if (strcmp(argv[0], "create") == 0) {
        int as_json = has_flag(argc - 1, argv + 1, "--json");
        int dry_run = has_flag(argc - 1, argv + 1, "--dry-run");
        int force = has_flag(argc - 1, argv + 1, "--force");
        int size = arg_int(argc - 1, argv + 1, "--size", 160);
        const char *path = arg_value(argc - 1, argv + 1, "--path");
        return create_one(path, size, force, dry_run, as_json, NULL, 0);
    }
    if (strcmp(argv[0], "session") == 0) return cmd_thumb_session(argc - 1, argv + 1);
    if (strcmp(argv[0], "status") == 0) return cmd_thumb_status(argc - 1, argv + 1);
    usage();
    fprintf(stderr, "[loogal:thumbnail_error] unknown thumbnail command: %s\n", argv[0]);
    return 1;
}
