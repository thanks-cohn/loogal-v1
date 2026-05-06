#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

cat > /tmp/loogal_sha256_test.c <<'C_EOF'
#include "loogal/sha256.h"

#include <stdio.h>
#include <string.h>

static int fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    return 1;
}

int main(void) {
    char hex[65];

    if (loogal_sha256_bytes_hex((const unsigned char *)"abc", 3, hex) != 0) {
        return fail("bytes hash failed");
    }

    if (strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") != 0) {
        fprintf(stderr, "got: %s\n", hex);
        return fail("abc hash mismatch");
    }

    FILE *f = fopen("/tmp/loogal-sha256-abc.txt", "wb");
    if (!f) return fail("could not create temp file");

    fwrite("abc", 1, 3, f);
    fclose(f);

    if (loogal_sha256_file_hex("/tmp/loogal-sha256-abc.txt", hex) != 0) {
        return fail("file hash failed");
    }

    if (strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") != 0) {
        fprintf(stderr, "got: %s\n", hex);
        return fail("file abc hash mismatch");
    }

    printf("NATIVE SHA256 TEST OK\n");
    printf("  abc: %s\n", hex);

    return 0;
}
C_EOF

cc -O2 -Wall -Wextra -std=c11 -Iinclude \
  /tmp/loogal_sha256_test.c src/core/sha256.c \
  -o /tmp/loogal_sha256_test

/tmp/loogal_sha256_test
