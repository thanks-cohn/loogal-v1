#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cat > /tmp/loogal_platform_test.c <<'C_EOF'
#include "loogal/platform.h"

#include <stdint.h>
#include <stdio.h>


static int walk_seen_file = 0;
static int walk_seen_dir = 0;

static int walk_cb(const LoogalPlatformWalkEntry *entry, void *user) {
(void)user;

if (!entry) return 1;

if (entry->type == LOOGAL_PLATFORM_ENTRY_FILE && entry->size == 6) {
walk_seen_file = 1;
}

if (entry->type == LOOGAL_PLATFORM_ENTRY_DIR) {
walk_seen_dir = 1;
}

return 0;
}


static int fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

int main(void) {
    const char *dir = "/tmp/loogal-platform-test";
    const char *src = "/tmp/loogal-platform-test/source.txt";
    const char *dst = "/tmp/loogal-platform-test/copy.txt";

const char *subdir = "/tmp/loogal-platform-test/subdir";
const char *nested = "/tmp/loogal-platform-test/subdir/nested.txt";

    if (loogal_platform_mkdir(dir) != 0) {
        return fail("mkdir failed");
    }

    if (!loogal_platform_dir_exists(dir)) {
        return fail("dir_exists failed");
    }

    FILE *f = fopen(src, "wb");
    if (!f) return fail("could not create source file");

    fwrite("loogal", 1, 6, f);
    fclose(f);

    if (!loogal_platform_file_exists(src)) {
        return fail("file_exists failed");
    }

    if (!loogal_platform_path_exists(src)) {
        return fail("path_exists file failed");
    }

    if (!loogal_platform_path_exists(dir)) {
        return fail("path_exists dir failed");
    }

    if (!loogal_platform_executable_exists("/bin/sh")) {
        return fail("executable_exists failed");
    }

    if (!loogal_platform_file_readable(src)) {
        return fail("file_readable failed");
    }

    uint64_t size = 0;
    if (loogal_platform_file_size(src, &size) != 0) {
        return fail("file_size failed");
    }

    if (size != 6) {
        fprintf(stderr, "size=%llu\n", (unsigned long long)size);
        return fail("file_size mismatch");
    }

    if (loogal_platform_copy_file(src, dst) != 0) {
        return fail("copy_file failed");
    }

    uint64_t copied_size = 0;
    if (loogal_platform_file_size(dst, &copied_size) != 0) {
        return fail("copied file_size failed");
    }

    if (copied_size != 6) {
        fprintf(stderr, "copied_size=%llu\n", (unsigned long long)copied_size);
        return fail("copied size mismatch");
    }

if (loogal_platform_mkdir(subdir) != 0) {
return fail("walk subdir mkdir failed");
}

FILE *nf = fopen(nested, "wb");
if (!nf) return fail("could not create nested file");

fwrite("loogal", 1, 6, nf);
fclose(nf);

if (loogal_platform_walk(dir, walk_cb, NULL) != 0) {
return fail("platform_walk failed");
}

if (!walk_seen_file) {
return fail("platform_walk did not see file");
}

if (!walk_seen_dir) {
return fail("platform_walk did not see dir");
}

    uint64_t now = loogal_platform_now_ns();
    if (now == 0) {
        return fail("now_ns returned zero");
    }

    printf("PLATFORM CORE TEST OK\n");
    printf("  file_size: %llu\n", (unsigned long long)size);
    printf("  now_ns:    %llu\n", (unsigned long long)now);

    return 0;
}
C_EOF

cc -O2 -Wall -Wextra -std=c11 -Iinclude \
    /tmp/loogal_platform_test.c src/platform.c \
    -o /tmp/loogal_platform_test

/tmp/loogal_platform_test
