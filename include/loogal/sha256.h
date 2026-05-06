#ifndef LOOGAL_SHA256_H
#define LOOGAL_SHA256_H

#include <stddef.h>
#include <stdint.h>

int loogal_sha256_bytes_hex(const unsigned char *data, size_t len, char out_hex[65]);
int loogal_sha256_file_hex(const char *path, char out_hex[65]);
int loogal_sha256_file(const char *path, char out_hex[65]);

#endif
