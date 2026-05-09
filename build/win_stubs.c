#include <stdio.h>
#include "loogal.h"

int cmd_action(int argc, char **argv) {
    (void)argc; (void)argv;
    fprintf(stderr, "cmd_action disabled in temporary Windows build\n");
    return 1;
}

int cmd_hash_v0(int argc, char **argv) {
    (void)argc; (void)argv;
    fprintf(stderr, "hash v0 disabled in temporary Windows build; use native\n");
    return 1;
}

int cmd_hash_v0_grid(int argc, char **argv) {
    (void)argc; (void)argv;
    fprintf(stderr, "hash v0 grid disabled in temporary Windows build; use native\n");
    return 1;
}

int cmd_hash_compare(int argc, char **argv) {
    (void)argc; (void)argv;
    fprintf(stderr, "hash compare disabled in temporary Windows build\n");
    return 1;
}

int image_probe_v0(const char *path, LoogalImageInfo *info) {
    (void)path;
    (void)info;
    return -1;
}
