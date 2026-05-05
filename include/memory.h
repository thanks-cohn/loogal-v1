#ifndef LOOGAL_MEMORY_H
#define LOOGAL_MEMORY_H

#include "loogal.h"
#include <stddef.h>
#include <stdint.h>

#define LOOGAL_IDENTITIES_PATH "data/identities.jsonl"
#define LOOGAL_LOCATIONS_PATH  "data/locations.jsonl"
#define LOOGAL_EVENTS_PATH     "data/events.jsonl"

#define LOOGAL_LOCATION_ACTIVE  1
#define LOOGAL_LOCATION_MISSING 2

typedef struct {
    uint64_t id;
    uint64_t dhash;
    char sha256[65];
    float aspect;
    uint64_t best_file_size;
    int32_t width;
    int32_t height;
    uint64_t location_count;
    uint64_t first_seen_unix;
    uint64_t last_seen_unix;
} LoogalIdentity;

typedef struct {
    uint64_t id;
    uint64_t identity_id;
    char path[LOOGAL_PATH_MAX];
    char kind[32];
    uint64_t file_size;
    uint64_t seen_unix;
    uint32_t status;
} LoogalLocation;

typedef struct {
    LoogalIdentity *identities;
    size_t identity_count;
    size_t identity_cap;

    LoogalLocation *locations;
    size_t location_count;
    size_t location_cap;

    size_t seen_new_identities;
    size_t seen_new_locations;
    size_t seen_known_locations;
    size_t seen_duplicate_locations;
    size_t seen_modified_paths;
} LoogalMemory;

void loogal_memory_init(LoogalMemory *m);
void loogal_memory_free(LoogalMemory *m);
int loogal_memory_load(LoogalMemory *m);
int loogal_memory_save(const LoogalMemory *m);
int loogal_memory_reset_files(void);
int loogal_memory_ingest_image(LoogalMemory *m, const LoogalImageInfo *info);
int loogal_memory_build_records(const LoogalMemory *m, LoogalRecord **out_records, size_t *out_count);
int loogal_memory_append_event(const char *type, const char *status, const char *path, uint64_t identity_id, const char *message);

#endif
