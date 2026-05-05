#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

int image_is_supported(const char *path) {
    const char *e = file_extension(path);
    return strcasecmp(e, "png") == 0 || strcasecmp(e, "jpg") == 0 ||
           strcasecmp(e, "jpeg") == 0 || strcasecmp(e, "webp") == 0 ||
           strcasecmp(e, "bmp") == 0 || strcasecmp(e, "tif") == 0 ||
           strcasecmp(e, "tiff") == 0;
}

int image_probe(const char *path, LoogalImageInfo *out) {
    if (!image_is_supported(path)) return -1;
    memset(out, 0, sizeof(*out));
    snprintf(out->path, sizeof(out->path), "%s", path);
    snprintf(out->ext, sizeof(out->ext), "%s", file_extension(path));
    out->file_size = file_size_bytes(path);

    char quoted[LOOGAL_PATH_MAX * 2 + 16];
    if (loogal_shell_quote(path, quoted, sizeof(quoted)) != 0) return -1;

    char cmd[LOOGAL_PATH_MAX * 2 + 256];
    snprintf(cmd, sizeof(cmd), "magick identify -format '%%w %%h' %s 2>/dev/null", quoted);
    FILE *p = popen(cmd, "r");
    if (!p) return -1;
    if (fscanf(p, "%d %d", &out->width, &out->height) != 2) {
        pclose(p);
        return -1;
    }
    pclose(p);
    if (out->width <= 0 || out->height <= 0) return -1;
    out->aspect = (float)out->width / (float)out->height;
    if (compute_dhash(path, &out->dhash) != 0) return -1;
    if (loogal_sha256_file(path, out->sha256) != 0) {
        memset(out->sha256, 0, sizeof(out->sha256));
        loogal_log("hash.sha256", "warn", path);
    }
    return 0;
}

int compute_dhash(const char *path, uint64_t *out_hash) {
    unsigned char px[9 * 8];
    char quoted[LOOGAL_PATH_MAX * 2 + 16];
    if (loogal_shell_quote(path, quoted, sizeof(quoted)) != 0) return -1;
    char cmd[LOOGAL_PATH_MAX * 2 + 512];
    snprintf(cmd, sizeof(cmd), "magick %s -auto-orient -resize 9x8! -colorspace Gray -depth 8 gray:- 2>/dev/null", quoted);
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

int hamming64(uint64_t a, uint64_t b) {
    uint64_t x = a ^ b;
    int c = 0;
    while (x) { c += x & 1ULL; x >>= 1; }
    return c;
}

int similarity_percent(uint64_t a, uint64_t b) {
    int d = hamming64(a, b);
    int pct = (int)((64 - d) * 100 / 64);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}
