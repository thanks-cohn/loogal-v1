#define _XOPEN_SOURCE 700

#include "loogal/index_service.h"
#include "loogal.h"
#include "memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int loogal_index_one_path(const char *path, int hash_mode_v0, int dry_run, LoogalIndexOneResult *out) {
    if (out) memset(out, 0, sizeof(*out));

    if (!path || !path[0]) {
        loogal_log("index.one", "error", "missing path");
        return 1;
    }

    if (!image_is_supported(path)) {
        if (out) out->skipped = 1;
        loogal_log("index.one", "skip", path);
        return 0;
    }

    struct stat st;
    if (stat(path, &st) != 0) {
        loogal_log("index.one", "error", path);
        return 1;
    }

    LoogalMemory memory;
    memset(&memory, 0, sizeof(memory));

    if (loogal_memory_load(&memory) != 0) {
        loogal_log("index.one", "error", "failed loading memory");
        return 1;
    }

    uint64_t file_size = (uint64_t)st.st_size;
    uint64_t mtime_unix = (uint64_t)st.st_mtime;

    if (!dry_run && loogal_memory_mark_known_location_if_unchanged(&memory, path, file_size, mtime_unix, 1)) {
        if (out) out->known_unchanged = 1;
        loogal_memory_free(&memory);
        return 0;
    }

    LoogalImageInfo info;
    if ((hash_mode_v0 ? image_probe_v0(path, &info) : image_probe(path, &info)) != 0) {
        loogal_memory_free(&memory);
        loogal_log("index.one", "error", path);
        return 1;
    }

    info.file_size = file_size;
    info.mtime_unix = mtime_unix;

    if (out) out->decoded = 1;

    if (!dry_run) {
        size_t before_id = memory.seen_new_identities;
        size_t before_loc = memory.seen_new_locations;
        size_t before_dup = memory.seen_duplicate_locations;
        size_t before_mod = memory.seen_modified_paths;

        loogal_memory_ingest_image(&memory, &info);

        int changed =
            memory.seen_new_identities != before_id ||
            memory.seen_new_locations != before_loc ||
            memory.seen_duplicate_locations != before_dup ||
            memory.seen_modified_paths != before_mod;

        if (out) out->changed = changed;

        if (changed) {
            LoogalRecord *records = NULL;
            size_t record_count = 0;

            if (loogal_memory_build_records(&memory, &records, &record_count) != 0) {
                loogal_memory_free(&memory);
                loogal_log("index.one", "error", "failed building records");
                return 1;
            }

            if (loogal_memory_save(&memory) != 0) {
                free(records);
                loogal_memory_free(&memory);
                loogal_log("index.one", "error", "failed saving memory");
                return 1;
            }

            if (write_index_records(records, record_count) != 0) {
                free(records);
                loogal_memory_free(&memory);
                loogal_log("index.one", "error", "failed writing binary index");
                return 1;
            }

            free(records);
        }
    }

    loogal_memory_free(&memory);
    return 0;
}
