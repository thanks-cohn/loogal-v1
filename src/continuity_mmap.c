#include "continuity_core.h"
#include "loogal.h"

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    uint64_t dhash;
    uint64_t offset;
} ManifestationIndexRecord;

void *continuity_mmap_index(
    const char *index_path,
    size_t *mapped_size
) {
    int fd = open(index_path, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return NULL;
    }

    void *map = mmap(
        NULL,
        st.st_size,
        PROT_READ,
        MAP_SHARED,
        fd,
        0
    );

    close(fd);

    if (map == MAP_FAILED) {
        return NULL;
    }

    *mapped_size = (size_t)st.st_size;

    return map;
}

int continuity_scan_index(
    const ManifestationIndexRecord *records,
    size_t count,
    uint64_t query_dhash,
    uint64_t *best_offset,
    int *best_distance
) {
    *best_distance = 65;
    *best_offset = 0;

    for (size_t i = 0; i < count; i++) {
        uint64_t delta = records[i].dhash ^ query_dhash;

        int dist = 0;
        while (delta) {
            dist += (delta & 1ULL);
            delta >>= 1ULL;
        }

        if (dist < *best_distance) {
            *best_distance = dist;
            *best_offset = records[i].offset;
        }
    }

    return (*best_distance <= 64);
}
