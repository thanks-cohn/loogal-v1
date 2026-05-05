#ifndef LOOGAL_HASH_H
#define LOOGAL_HASH_H

#include <stddef.h>

int loogal_sha256_file(const char *path, char out_hex[65]);
int loogal_shell_quote(const char *in, char *out, size_t out_sz);

#endif
