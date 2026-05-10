#include "shard_index.h"
#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("locus-shard index");
    puts("locus-shard find <crop.png> [--limit N] [--min N]");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    if (strcmp(argv[1], "index") == 0) {
        return cmd_shard_index(argc - 2, argv + 2);
    }

    if (strcmp(argv[1], "find") == 0) {
        return cmd_shard_find(argc - 2, argv + 2);
    }

    usage();
    return 1;
}
