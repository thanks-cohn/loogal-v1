#ifndef LOOGAL_SHARD_INDEX_H
#define LOOGAL_SHARD_INDEX_H

#include <stdint.h>

#define LOOGAL_SHARD_INDEX_MAGIC "LSHARD1"
#define LOOGAL_SHARD_TILE_SIZE 96
#define LOOGAL_SHARD_STRIDE 48

typedef struct {
    uint64_t dhash;
    uint64_t ahash;
    uint32_t image_id;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
} LoogalShardTile;

int cmd_shard_index(int argc, char **argv);
int cmd_shard_find(int argc, char **argv);

#endif
