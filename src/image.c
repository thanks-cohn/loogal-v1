#define _XOPEN_SOURCE 700

#include "loogal.h"
#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_WEBP
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#include "../vendor/stb_image.h"

#define LOOGAL_DHASH_W 9
#define LOOGAL_DHASH_H 8
#define LOOGAL_AHASH_W 8
#define LOOGAL_AHASH_H 8

int image_is_supported(const char *path) {
    const char *e = file_extension(path);

    return strcasecmp(e, "png") == 0 ||
           strcasecmp(e, "jpg") == 0 ||
           strcasecmp(e, "jpeg") == 0 ||
           strcasecmp(e, "webp") == 0 ||
           strcasecmp(e, "bmp") == 0 ||
           strcasecmp(e, "gif") == 0;
}

static unsigned char sample_box_average(
    const unsigned char *gray,
    int width,
    int height,
    int tx,
    int ty,
    int out_w,
    int out_h
) {
    int x0 = (tx * width) / out_w;
    int x1 = ((tx + 1) * width) / out_w;
    int y0 = (ty * height) / out_h;
    int y1 = ((ty + 1) * height) / out_h;

    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > width) x1 = width;
    if (y1 > height) y1 = height;

    unsigned long sum = 0;
    unsigned long count = 0;

    for (int y = y0; y < y1; y++) {
        const unsigned char *row = gray + ((size_t)y * (size_t)width);

        for (int x = x0; x < x1; x++) {
            sum += row[x];
            count++;
        }
    }

    if (count == 0) return 0;

    return (unsigned char)(sum / count);
}

static int make_dhash_from_gray(
    const unsigned char *gray,
    int width,
    int height,
    uint64_t *out_hash
) {
    if (!gray || width <= 0 || height <= 0 || !out_hash) return -1;

    unsigned char small[LOOGAL_DHASH_W * LOOGAL_DHASH_H];

    for (int y = 0; y < LOOGAL_DHASH_H; y++) {
        for (int x = 0; x < LOOGAL_DHASH_W; x++) {
            small[y * LOOGAL_DHASH_W + x] = sample_box_average(
                gray,
                width,
                height,
                x,
                y,
                LOOGAL_DHASH_W,
                LOOGAL_DHASH_H
            );
        }
    }

    uint64_t h = 0;

    for (int y = 0; y < LOOGAL_DHASH_H; y++) {
        for (int x = 0; x < LOOGAL_DHASH_W - 1; x++) {
            unsigned char left = small[y * LOOGAL_DHASH_W + x];
            unsigned char right = small[y * LOOGAL_DHASH_W + x + 1];

            h <<= 1;

            if (left > right) {
                h |= 1ULL;
            }
        }
    }

    *out_hash = h;
    return 0;
}


static int make_ahash_from_gray(
    const unsigned char *gray,
    int width,
    int height,
    uint64_t *out_hash
) {
    if (!gray || width <= 0 || height <= 0 || !out_hash) return -1;

    unsigned char small[LOOGAL_AHASH_W * LOOGAL_AHASH_H];
    unsigned long sum = 0;

    for (int y = 0; y < LOOGAL_AHASH_H; y++) {
        for (int x = 0; x < LOOGAL_AHASH_W; x++) {
            unsigned char v = sample_box_average(
                gray,
                width,
                height,
                x,
                y,
                LOOGAL_AHASH_W,
                LOOGAL_AHASH_H
            );

            small[y * LOOGAL_AHASH_W + x] = v;
            sum += v;
        }
    }

    unsigned char avg = (unsigned char)(sum / (LOOGAL_AHASH_W * LOOGAL_AHASH_H));
    uint64_t h = 0;

    for (int i = 0; i < LOOGAL_AHASH_W * LOOGAL_AHASH_H; i++) {
        h <<= 1;

        if (small[i] > avg) {
            h |= 1ULL;
        }
    }

    *out_hash = h;
    return 0;
}

int compute_dhash(const char *path, uint64_t *out_hash) {
    if (!path || !out_hash) return -1;

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char *gray = stbi_load(path, &width, &height, &channels, 1);

    if (!gray) {
        char msg[512];
        snprintf(msg, sizeof(msg), "stb decode failed: %.360s", path);
        loogal_log("image.decode", "error", msg);
        return -1;
    }

    int rc = make_dhash_from_gray(gray, width, height, out_hash);

    stbi_image_free(gray);

    return rc;
}

int compute_ahash(const char *path, uint64_t *out_hash) {
    if (!path || !out_hash) return -1;

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char *gray = stbi_load(path, &width, &height, &channels, 1);

    if (!gray) {
        char msg[512];
        snprintf(msg, sizeof(msg), "stb decode failed: %.360s", path);
        loogal_log("image.decode", "error", msg);
        return -1;
    }

    int rc = make_ahash_from_gray(gray, width, height, out_hash);

    stbi_image_free(gray);

    return rc;
}

int image_probe(const char *path, LoogalImageInfo *out) {
    if (!path || !out) return -1;
    if (!image_is_supported(path)) return -1;

    memset(out, 0, sizeof(*out));

    snprintf(out->path, sizeof(out->path), "%s", path);
    snprintf(out->ext, sizeof(out->ext), "%s", file_extension(path));

    out->file_size = file_size_bytes(path);

    int width = 0;
    int height = 0;
    int channels = 0;

    if (!stbi_info(path, &width, &height, &channels)) {
        char msg[512];
        snprintf(msg, sizeof(msg), "stb info failed: %.360s", path);
        loogal_log("image.info", "error", msg);
        return -1;
    }

    if (width <= 0 || height <= 0) return -1;

    out->width = width;
    out->height = height;
    out->aspect = (float)width / (float)height;

    if (compute_dhash(path, &out->dhash) != 0) {
        return -1;
    }

    if (compute_ahash(path, &out->ahash) != 0) {
        return -1;
    }

    /*
       TODO:
       This calls the native C SHA-256 helper.
       The ImageMagick dependency is gone from the image primitive.
       Next steel plate: replace hash.c with native C SHA-256.
    */
    if (loogal_sha256_file(path, out->sha256) != 0) {
        memset(out->sha256, 0, sizeof(out->sha256));
        loogal_log("hash.sha256", "warn", path);
    }

    return 0;
}

int hamming64(uint64_t a, uint64_t b) {
    uint64_t x = a ^ b;

#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(x);
#else
    int c = 0;

    while (x) {
        c += (int)(x & 1ULL);
        x >>= 1;
    }

    return c;
#endif
}

int similarity_percent(uint64_t a, uint64_t b) {
    int d = hamming64(a, b);
    int pct = (int)((64 - d) * 100 / 64);

    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;

    return pct;
}
