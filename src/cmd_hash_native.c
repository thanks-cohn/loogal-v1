#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "jsonout.h"
#include "hash_native.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("LOOGAL HASH-NATIVE — native C image hash inspection");
    puts("");
    puts("Usage:");
    puts("  loogal hash-native <image>");
    puts("  loogal hash-native <image> --json");
}

int cmd_hash_native(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        usage();
        return 0;
    }

    const char *path = argv[0];
    int as_json = loogal_has_flag(argc, argv, "--json");

    LoogalImageInfo info;
    if (image_probe(path, &info) != 0) {
        fprintf(stderr, "[loogal:hash_native_error] image_probe failed: %s\n", path);
        LOOGAL_ERROR(LOOGAL_ERR_IMAGE_PROBE_FAILED,
                     "hash_native",
                     "image_probe",
                     path,
                     "native image probe failed");
        return 1;
    }

    if (as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "hash.native", 1);
        printf("  "); loogal_json_kv_string(stdout, "path", info.path, 1);
        printf("  "); loogal_json_kv_string(stdout, "mode", "native-c-current", 1);
        printf("  "); loogal_json_kv_string(stdout, "ext", info.ext, 1);
        printf("  "); loogal_json_kv_int(stdout, "width", info.width, 1);
        printf("  "); loogal_json_kv_int(stdout, "height", info.height, 1);
        printf("  \"aspect\": %.9f,\n", (double)info.aspect);
        printf("  \"file_size_bytes\": %llu,\n", (unsigned long long)info.file_size);
        printf("  \"mtime_unix\": %llu,\n", (unsigned long long)info.mtime_unix);
        printf("  \"dhash_u64\": %llu,\n", (unsigned long long)info.dhash);
        printf("  \"dhash_hex\": \"0x%016llx\",\n", (unsigned long long)info.dhash);
        printf("  \"ahash_u64\": %llu,\n", (unsigned long long)info.ahash);
        printf("  \"ahash_hex\": \"0x%016llx\",\n", (unsigned long long)info.ahash);
        printf("  "); loogal_json_kv_string(stdout, "pipeline", "stb_image grayscale + native resize/hash current", 0);
        puts("}");
    } else {
        printf("[loogal:hash_native]\n");
        printf("  path:   %s\n", info.path);
        printf("  mode:   native-c-current\n");
        printf("  size:   %dx%d\n", info.width, info.height);
        printf("  dhash:  0x%016llx\n", (unsigned long long)info.dhash);
        printf("  ahash:  0x%016llx\n", (unsigned long long)info.ahash);
    }

    LOOGAL_INFO("hash_native", "computed", path, "computed native C image hashes");
    return 0;
}


#define STB_IMAGE_IMPLEMENTATION_NATIVE_GRID_DISABLED 1
#include "../vendor/stb_image.h"

static unsigned char native_grid_box_average(
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

int cmd_hash_native_grid(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        puts("Usage: loogal hash-native-grid <image> --json");
        return 0;
    }

    const char *path = argv[0];
    int as_json = loogal_has_flag(argc, argv, "--json");

    int width = 0;
    int height = 0;
    int channels = 0;

    unsigned char *gray = stbi_load(path, &width, &height, &channels, 1);
    if (!gray) {
        fprintf(stderr, "[loogal:hash_native_grid_error] stb decode failed: %s\n", path);
        return 1;
    }

    unsigned char px[9 * 8];

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 9; x++) {
            px[y * 9 + x] = native_grid_box_average(gray, width, height, x, y, 9, 8);
        }
    }

    stbi_image_free(gray);

    if (as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "hash.native.grid", 1);
        printf("  "); loogal_json_kv_string(stdout, "path", path, 1);
        printf("  \"source_width\": %d,\n", width);
        printf("  \"source_height\": %d,\n", height);
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
        puts("[loogal:hash_native_grid]");
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
