#ifndef LOOGAL_PLATFORM_H
#define LOOGAL_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

int loogal_platform_file_size(const char *path, uint64_t *out_size);
int loogal_platform_file_metadata(const char *path, uint64_t *out_size, uint64_t *out_mtime);
int loogal_platform_file_exists(const char *path);
int loogal_platform_file_readable(const char *path);
int loogal_platform_mkdir(const char *path);
int loogal_platform_copy_file(const char *src, const char *dst);
uint64_t loogal_platform_now_ns(void);

#endif
