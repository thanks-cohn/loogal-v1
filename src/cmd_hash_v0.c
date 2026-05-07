#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "jsonout.h"
#include "hash_v0.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static int quote_sh(const char *in, char *out, size_t out_sz) {
    if (!in || !out || out_sz < 3) return -1;

    size_t j = 0;
    out[j++] = '\'';

    for (size_t i = 0; in[i]; i++) {
        if (in[i] == '\'') {
            const char *esc = "'\\''";
            for (size_t k = 0; esc[k]; k++) {
                if (j + 1 >= out_sz) return -1;
                out[j++] = esc[k];
            }
        } else {
            if (j + 1 >= out_sz) return -1;
            out[j++] = in[i];
        }
    }

    if (j + 1 >= out_sz) return -1;
    out[j++] = '\'';
    out[j] = 0;
    return 0;
}

static int hamming64_local(uint64_t a, uint64_t b) {
    uint64_t x = a ^ b;
    int c = 0;
    while (x) {
        c += (int)(x & 1ULL);
        x >>= 1;
    }
    return c;
}

static int magick_identify_dimensions(const char *path, int *w, int *h) {
    char q[LOOGAL_PATH_MAX * 2];
    if (quote_sh(path, q, sizeof(q)) != 0) return -1;

    char cmd[LOOGAL_PATH_MAX * 3];
    int n = snprintf(cmd, sizeof(cmd),
                     "magick identify -format '%%w %%h' %s 2>/dev/null",
                     q);
    if (n < 0 || n >= (int)sizeof(cmd)) return -1;

    FILE *p = popen(cmd, "r");
    if (!p) return -1;

    int ok = fscanf(p, "%d %d", w, h);
    int rc = pclose(p);

    if (ok != 2 || rc != 0) return -1;
    if (*w <= 0 || *h <= 0) return -1;

    return 0;
}

static int magick_v0_dhash(const char *path, uint64_t *out_hash) {
    if (!path || !out_hash) return -1;

    unsigned char px[9 * 8];

    char q[LOOGAL_PATH_MAX * 2];
    if (quote_sh(path, q, sizeof(q)) != 0) return -1;

    char cmd[LOOGAL_PATH_MAX * 3];
    int n = snprintf(cmd, sizeof(cmd),
                     "magick %s -auto-orient -resize 9x8! -colorspace Gray -depth 8 gray:- 2>/dev/null",
                     q);
    if (n < 0 || n >= (int)sizeof(cmd)) return -1;

    FILE *p = popen(cmd, "r");
    if (!p) return -1;

    size_t got = fread(px, 1, sizeof(px), p);
    int rc = pclose(p);

    if (got != sizeof(px) || rc != 0) return -1;

    uint64_t h = 0;

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            unsigned char left = px[y * 9 + x];
            unsigned char right = px[y * 9 + x + 1];

            h <<= 1;
            if (left > right) h |= 1ULL;
        }
    }

    *out_hash = h;
    return 0;
}

static void usage(void) {
    puts("LOOGAL HASH-V0 — ImageMagick-compatible old Loogal dHash");
    puts("");
    puts("Usage:");
    puts("  loogal hash-v0 <image>");
    puts("  loogal hash-v0 <image> --json");
    puts("");
    puts("Purpose:");
    puts("  Reproduces the original Loogal v0 ImageMagick dHash route:");
    puts("  magick image -auto-orient -resize 9x8! -colorspace Gray -depth 8 gray:-");
}

int cmd_hash_v0(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        usage();
        return 0;
    }

    const char *path = argv[0];
    int as_json = loogal_has_flag(argc, argv, "--json");

    int width = 0;
    int height = 0;
    uint64_t dhash = 0;

    if (magick_identify_dimensions(path, &width, &height) != 0) {
        fprintf(stderr, "[loogal:hash_v0_error] magick identify failed: %s\n", path);
        LOOGAL_ERROR(LOOGAL_ERR_IMAGE_PROBE_FAILED,
                     "hash_v0",
                     "identify",
                     path,
                     "ImageMagick identify failed");
        return 1;
    }

    if (magick_v0_dhash(path, &dhash) != 0) {
        fprintf(stderr, "[loogal:hash_v0_error] magick dHash failed: %s\n", path);
        LOOGAL_ERROR(LOOGAL_ERR_IMAGE_HASH_FAILED,
                     "hash_v0",
                     "dhash",
                     path,
                     "ImageMagick v0-compatible dHash failed");
        return 1;
    }

    if (as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "hash.v0.magick", 1);
        printf("  "); loogal_json_kv_string(stdout, "path", path, 1);
        printf("  "); loogal_json_kv_string(stdout, "mode", "magick-v0-compatible", 1);
        printf("  "); loogal_json_kv_int(stdout, "width", width, 1);
        printf("  "); loogal_json_kv_int(stdout, "height", height, 1);
        printf("  \"aspect\": %.9f,\n", height > 0 ? (double)width / (double)height : 0.0);
        printf("  \"dhash_u64\": %llu,\n", (unsigned long long)dhash);
        printf("  \"dhash_hex\": \"0x%016llx\",\n", (unsigned long long)dhash);
        printf("  \"self_hamming\": %d,\n", hamming64_local(dhash, dhash));
        printf("  "); loogal_json_kv_string(stdout, "pipeline", "magick identify + auto-orient + resize 9x8! + grayscale depth8 + dhash", 0);
        puts("}");
    } else {
        printf("[loogal:hash_v0]\n");
        printf("  path:      %s\n", path);
        printf("  mode:      magick-v0-compatible\n");
        printf("  size:      %dx%d\n", width, height);
        printf("  dhash:     0x%016llx\n", (unsigned long long)dhash);
    }

    LOOGAL_INFO("hash_v0", "computed", path, "computed ImageMagick-compatible v0 dHash");
    return 0;
}


int cmd_hash_v0_grid(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        puts("Usage: loogal hash-v0-grid <image> --json");
        return 0;
    }

    const char *path = argv[0];
    int as_json = loogal_has_flag(argc, argv, "--json");

    unsigned char px[9 * 8];

    char q[LOOGAL_PATH_MAX * 2];
    if (quote_sh(path, q, sizeof(q)) != 0) return 1;

    char cmd[LOOGAL_PATH_MAX * 3];
    int n = snprintf(cmd, sizeof(cmd),
                     "magick %s -auto-orient -resize 9x8! -colorspace Gray -depth 8 gray:- 2>/dev/null",
                     q);
    if (n < 0 || n >= (int)sizeof(cmd)) return 1;

    FILE *p = popen(cmd, "r");
    if (!p) return 1;

    size_t got = fread(px, 1, sizeof(px), p);
    int rc = pclose(p);

    if (got != sizeof(px) || rc != 0) return 1;

    if (as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "hash.v0.grid", 1);
        printf("  "); loogal_json_kv_string(stdout, "path", path, 1);
        printf("  \"width\": 9,\n");
        printf("  \"height\": 8,\n");
        printf("  \"bytes\": [");
        for (int i = 0; i < 72; i++) {
            if (i) printf(",");
            printf("%u", (unsigned)px[i]);
        }
        printf("]\n");
        puts("}");
    } else {
        puts("[loogal:hash_v0_grid]");
        for (int y = 0; y < 8; y++) {
            printf("  ");
            for (int x = 0; x < 9; x++) {
                printf("%3u%s", (unsigned)px[y * 9 + x], x == 8 ? "" : " ");
            }
            putchar('\n');
        }
    }

    return 0;
}


int cmd_hash_compare(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        puts("Usage: loogal hash-compare <image> --json");
        return 0;
    }

    const char *path = argv[0];
    int as_json = loogal_has_flag(argc, argv, "--json");

    uint64_t v0 = 0;
    uint64_t native = 0;

    if (magick_v0_dhash(path, &v0) != 0) {
        fprintf(stderr, "[loogal:hash_compare_error] v0 Magick hash failed: %s\n", path);
        return 1;
    }

    if (compute_dhash(path, &native) != 0) {
        fprintf(stderr, "[loogal:hash_compare_error] native hash failed: %s\n", path);
        return 1;
    }

    int dist = hamming64_local(v0, native);
    int matching = 64 - dist;
    double similarity = (double)matching / 64.0;

    if (as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "hash.compare", 1);
        printf("  "); loogal_json_kv_string(stdout, "path", path, 1);
        printf("  \"v0_dhash_hex\": \"0x%016llx\",\n", (unsigned long long)v0);
        printf("  \"native_dhash_hex\": \"0x%016llx\",\n", (unsigned long long)native);
        printf("  \"hamming_distance\": %d,\n", dist);
        printf("  \"matching_bits\": %d,\n", matching);
        printf("  \"similarity\": %.6f,\n", similarity);
        printf("  \"similarity_percent\": %.3f\n", similarity * 100.0);
        puts("}");
    } else {
        printf("[loogal:hash_compare]\n");
        printf("  path:              %s\n", path);
        printf("  v0_dhash:          0x%016llx\n", (unsigned long long)v0);
        printf("  native_dhash:      0x%016llx\n", (unsigned long long)native);
        printf("  hamming_distance:  %d\n", dist);
        printf("  matching_bits:     %d / 64\n", matching);
        printf("  similarity:        %.3f%%\n", similarity * 100.0);
    }

    return 0;
}
