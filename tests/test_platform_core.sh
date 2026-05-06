#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cat > /tmp/loogal_platform_test.c <<'C_EOF'
#include "loogal/platform.h"

#include <stdint.h>
#include <stdio.h>

static int fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

int main(void) {
    const char *dir = "/tmp/loogal-platform-test";
    const char *src = "/tmp/loogal-platform-test/source.txt";
    const char *dst = "/tmp/loogal-platform-test/copy.txt";

    if (loogal_platform_mkdir(dir) != 0) {
        return fail("mkdir failed");
    }

    FILE *f = fopen(src, "wb");
    if (!f) return fail("could not create source file");

    fwrite("loogal", 1, 6, f);
    fclose(f);

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
