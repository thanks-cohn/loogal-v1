#define _POSIX_C_SOURCE 200809L

#include "loogal/platform.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

int loogal_platform_file_size(const char *path, uint64_t *out_size) {
    if (!path || !out_size) return -1;

    struct stat st;

    if (stat(path, &st) != 0) {
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        return -1;
    }

    if (st.st_size < 0) {
        return -1;
    }

    *out_size = (uint64_t)st.st_size;
    return 0;
}

int loogal_platform_mkdir(const char *path) {
    if (!path || !path[0]) return -1;

    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    if (errno == EEXIST) {
        return 0;
    }

    return -1;
}

int loogal_platform_copy_file(const char *src, const char *dst) {
    if (!src || !dst) return -1;

    FILE *in = fopen(src, "rb");
    if (!in) return -1;

    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }

    unsigned char buf[65536];

    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), in);

        if (n > 0) {
            if (fwrite(buf, 1, n, out) != n) {
                fclose(in);
                fclose(out);
                return -1;
            }
        }

        if (n < sizeof(buf)) {
            if (ferror(in)) {
                fclose(in);
                fclose(out);
                return -1;
            }

            break;
        }
    }

    if (fclose(in) != 0) {
        fclose(out);
        return -1;
    }

    if (fclose(out) != 0) {
        return -1;
    }

    return 0;
}

uint64_t loogal_platform_now_ns(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
    }
#endif

    return (uint64_t)time(NULL) * 1000000000ULL;
}
