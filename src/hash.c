#define _XOPEN_SOURCE 700
#include "hash.h"
#include "loogal.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int loogal_shell_quote(const char *in, char *out, size_t out_sz) {
    if (!in || !out || out_sz < 3) return -1;
    size_t j = 0;
    out[j++] = '\'';
    for (size_t i = 0; in[i]; i++) {
        if (j + 5 >= out_sz) return -1;
        if (in[i] == '\'') {
            out[j++] = '\'';
            out[j++] = '\\';
            out[j++] = '\'';
            out[j++] = '\'';
        } else {
            out[j++] = in[i];
        }
    }
    if (j + 2 > out_sz) return -1;
    out[j++] = '\'';
    out[j] = 0;
    return 0;
}

int loogal_sha256_file(const char *path, char out_hex[65]) {
    if (!path || !out_hex) return -1;
    char quoted[LOOGAL_PATH_MAX * 2 + 16];
    if (loogal_shell_quote(path, quoted, sizeof(quoted)) != 0) return -1;

    char cmd[LOOGAL_PATH_MAX * 2 + 128];
    snprintf(cmd, sizeof(cmd), "sha256sum %s 2>/dev/null", quoted);

    FILE *p = popen(cmd, "r");
    if (!p) return -1;

    char buf[128] = {0};
    if (!fgets(buf, sizeof(buf), p)) {
        pclose(p);
        return -1;
    }
    int rc = pclose(p);
    if (rc != 0) return -1;

    for (int i = 0; i < 64; i++) {
        if (!isxdigit((unsigned char)buf[i])) return -1;
        out_hex[i] = (char)tolower((unsigned char)buf[i]);
    }
    out_hex[64] = 0;
    return 0;
}
