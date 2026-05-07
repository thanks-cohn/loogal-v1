#ifndef LOOGAL_PLATFORM_H
#define LOOGAL_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

int loogal_platform_file_size(const char *path, uint64_t *out_size);
int loogal_platform_file_metadata(const char *path, uint64_t *out_size, uint64_t *out_mtime);
int loogal_platform_file_exists(const char *path);
int loogal_platform_path_exists(const char *path);
int loogal_platform_executable_exists(const char *path);
int loogal_platform_dir_exists(const char *path);
int loogal_platform_file_readable(const char *path);
int loogal_platform_mkdir(const char *path);
int loogal_platform_copy_file(const char *src, const char *dst);

typedef enum {
    LOOGAL_PLATFORM_ENTRY_UNKNOWN = 0,
    LOOGAL_PLATFORM_ENTRY_FILE,
    LOOGAL_PLATFORM_ENTRY_DIR,
    LOOGAL_PLATFORM_ENTRY_SYMLINK,
    LOOGAL_PLATFORM_ENTRY_OTHER
} LoogalPlatformEntryType;

typedef struct {
    char path[4096];
    LoogalPlatformEntryType type;
    uint64_t size;
    uint64_t depth;
} LoogalPlatformWalkEntry;

typedef int (*LoogalPlatformWalkFn)(const LoogalPlatformWalkEntry *entry, void *user);

int loogal_platform_walk(const char *root, LoogalPlatformWalkFn fn, void *user);

uint64_t loogal_platform_now_ns(void);

#endif
