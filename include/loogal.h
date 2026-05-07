#ifndef LOOGAL_H
#define LOOGAL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define LOOGAL_VERSION "0.1.0"
#define LOOGAL_MAGIC "LOOGALV1"
#define LOOGAL_PATH_MAX 1024
#define LOOGAL_DATA_DIR "data"
#define LOOGAL_BIN_PATH "data/loogal.bin"
#define LOOGAL_RECORDS_PATH "data/records.jsonl"
#define LOOGAL_IDENTITIES_PATH "data/identities.jsonl"
#define LOOGAL_LOCATIONS_PATH "data/locations.jsonl"
#define LOOGAL_EVENTS_PATH "data/events.jsonl"
#define LOOGAL_LOG_PATH "data/logs/loogal.jsonl"
#define LOOGAL_MANIFEST_DIR "data/manifests"

#pragma pack(push, 1)
typedef struct {
    char magic[8];
    uint32_t version;
    uint64_t count;
    uint64_t created_unix;
} LoogalHeader;

typedef struct {
    uint64_t id;
    uint64_t dhash;
    uint64_t ahash;
    float aspect;
    uint64_t file_size;
    int32_t width;
    int32_t height;
    char path[LOOGAL_PATH_MAX];
} LoogalRecord;
#pragma pack(pop)

typedef struct {
char path[LOOGAL_PATH_MAX];
int width;
int height;
uint64_t file_size;
uint64_t mtime_unix;
uint64_t dhash;
uint64_t ahash;
char sha256[65];
float aspect;
char ext[16];
} LoogalImageInfo;

void loogal_log(const char *event, const char *status, const char *message);
void loogal_die(const char *event, const char *message);
int ensure_dirs(void);
int cmd_index(int argc, char **argv);
int cmd_search(int argc, char **argv);
int cmd_stats(int argc, char **argv);
int cmd_dedupe(int argc, char **argv);
int cmd_thumbnail(int argc, char **argv);
int cmd_action(int argc, char **argv);
int cmd_session(int argc, char **argv);
int image_probe(const char *path, LoogalImageInfo *out);
int image_is_supported(const char *path);
int compute_dhash(const char *path, uint64_t *out_hash);
int compute_ahash(const char *path, uint64_t *out_hash);
int write_index_records(LoogalRecord *records, size_t count);
int read_index_records(LoogalRecord **records, size_t *count);
int hamming64(uint64_t a, uint64_t b);
int similarity_percent(uint64_t a, uint64_t b);
uint64_t file_size_bytes(const char *path);
const char *file_extension(const char *path);
char *json_escape(const char *s);
long now_unix(void);
void iso_time_now(char *buf, size_t n);

int cmd_bench_scan(int argc, char **argv);
#endif
