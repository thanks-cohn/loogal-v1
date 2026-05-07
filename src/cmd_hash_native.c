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
