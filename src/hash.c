#include "loogal.h"
#include "loogal/sha256.h"

#include <stdio.h>

int compute_sha256(const char *path, char out_hex[65]) {
    if (!path || !out_hex) return -1;

    if (loogal_sha256_file_hex(path, out_hex) != 0) {
        fprintf(stderr, "[loogal:hash_error] native sha256 failed: %s\n", path);
        return -1;
    }

    return 0;
}
