#include "loogal.h"
#include "shard_index.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOGAL_SHARD_INDEX_PATH "data/shards.bin"
#define LOOGAL_SHARD_MAP_PATH   "data/shards.paths"

typedef struct {
    char magic[8];
    uint32_t version;
    uint32_t tile_size;
    uint32_t stride;
    uint64_t tile_count;
} LoogalShardHeader;

typedef struct {
    uint64_t dhash;
    uint64_t ahash;
    uint32_t image_id;
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
} ShardDiskTile;

typedef struct {
    ShardDiskTile tile;
    double score;
    int dhash_distance;
    int ahash_distance;
} ShardHit;

static double sim64(int dist) {
    if (dist < 0 || dist > 64) return 0.0;
    return 1.0 - ((double)dist / 64.0);
}

static double shard_score(int dd, int ad) {
    return (sim64(dd) * 0.70) + (sim64(ad) * 0.30);
}

static int hit_cmp(const void *a, const void *b) {
    const ShardHit *ha = (const ShardHit *)a;
    const ShardHit *hb = (const ShardHit *)b;
    if (ha->score < hb->score) return 1;
    if (ha->score > hb->score) return -1;
    if (ha->dhash_distance != hb->dhash_distance) return ha->dhash_distance - hb->dhash_distance;
    return ha->ahash_distance - hb->ahash_distance;
}

static int write_path_map(FILE *map, uint32_t image_id, const char *path) {
    return fprintf(map, "%u\t%s\n", image_id, path) > 0 ? 0 : 1;
}

static int load_path_for_id(uint32_t wanted, char *out, size_t out_sz) {
    FILE *f = fopen(LOOGAL_SHARD_MAP_PATH, "r");
    if (!f) return 1;

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        unsigned id = 0;
        char path[3500];
        path[0] = 0;
        if (sscanf(line, "%u\t%3499[^\n]", &id, path) == 2 && id == wanted) {
            snprintf(out, out_sz, "%s", path);
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return 1;
}

static int build_from_existing_index(void) {
    LoogalRecord *records = NULL;
    size_t record_count = 0;

    if (read_index_records(&records, &record_count) != 0) {
        loogal_die("shard-index", "could not read existing loogal binary index; run loogal index first");
        return 1;
    }

    FILE *out = fopen(LOOGAL_SHARD_INDEX_PATH, "wb");
    if (!out) {
        free(records);
        loogal_die("shard-index", "could not open shard index for writing");
        return 1;
    }

    FILE *map = fopen(LOOGAL_SHARD_MAP_PATH, "w");
    if (!map) {
        fclose(out);
        free(records);
        loogal_die("shard-index", "could not open shard path map for writing");
        return 1;
    }

    LoogalShardHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, LOOGAL_SHARD_INDEX_MAGIC, 7);
    header.version = 1;
    header.tile_size = LOOGAL_SHARD_TILE_SIZE;
    header.stride = LOOGAL_SHARD_STRIDE;
    header.tile_count = 0;

    fwrite(&header, sizeof(header), 1, out);

    for (size_t i = 0; i < record_count; i++) {
        write_path_map(map, (uint32_t)i, records[i].path);

        /* v0 fast path: one whole-record tile per indexed image.
           Next step replaces this with real overlapping decoded tiles. */
        ShardDiskTile t;
        memset(&t, 0, sizeof(t));
        t.dhash = records[i].dhash;
        t.ahash = records[i].ahash;
        t.image_id = (uint32_t)i;
        t.x = 0;
        t.y = 0;
        t.w = (uint32_t)(records[i].width > 0 ? records[i].width : 0);
        t.h = (uint32_t)(records[i].height > 0 ? records[i].height : 0);
        fwrite(&t, sizeof(t), 1, out);
        header.tile_count++;
    }

    fseek(out, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, out);

    fclose(map);
    fclose(out);
    free(records);

    printf("SHARD INDEX BUILT\n");
    printf("tiles:    %llu\n", (unsigned long long)header.tile_count);
    printf("tile:     %u\n", header.tile_size);
    printf("stride:   %u\n", header.stride);
    printf("index:    %s\n", LOOGAL_SHARD_INDEX_PATH);
    printf("paths:    %s\n", LOOGAL_SHARD_MAP_PATH);

    loogal_log("shard-index", "ok", LOOGAL_SHARD_INDEX_PATH);
    return 0;
}

int cmd_shard_index(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return build_from_existing_index();
}

int cmd_shard_find(int argc, char **argv) {
    if (argc < 1) {
        loogal_die("shard-find", "usage: loogal shard-find <crop.png> [--limit N] [--min N]");
        return 1;
    }

    const char *query_path = argv[0];
    int limit = 20;
    double min_percent = 55.0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) limit = atoi(argv[++i]);
        else if (strcmp(argv[i], "--min") == 0 && i + 1 < argc) min_percent = atof(argv[++i]);
    }
    if (limit <= 0) limit = 20;
    if (limit > 200) limit = 200;

    LoogalImageInfo query;
    if (image_probe(query_path, &query) != 0) {
        loogal_die("shard-find", "could not read query shard");
        return 1;
    }

    FILE *f = fopen(LOOGAL_SHARD_INDEX_PATH, "rb");
    if (!f) {
        loogal_die("shard-find", "missing shard index; run loogal shard-index after loogal index");
        return 1;
    }

    LoogalShardHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1 || memcmp(header.magic, LOOGAL_SHARD_INDEX_MAGIC, 7) != 0) {
        fclose(f);
        loogal_die("shard-find", "invalid shard index");
        return 1;
    }

    ShardHit *hits = calloc((size_t)limit, sizeof(ShardHit));
    if (!hits) {
        fclose(f);
        loogal_die("shard-find", "out of memory");
        return 1;
    }

    double start = loogal_now_ms();
    int kept = 0;

    for (uint64_t i = 0; i < header.tile_count; i++) {
        ShardDiskTile t;
        if (fread(&t, sizeof(t), 1, f) != 1) break;

        int dd = hamming64(query.dhash, t.dhash);
        int ad = hamming64(query.ahash, t.ahash);
        double score = shard_score(dd, ad);
        if ((score * 100.0) < min_percent) continue;

        ShardHit candidate;
        candidate.tile = t;
        candidate.score = score;
        candidate.dhash_distance = dd;
        candidate.ahash_distance = ad;

        if (kept < limit) {
            hits[kept++] = candidate;
            qsort(hits, kept, sizeof(ShardHit), hit_cmp);
        } else if (candidate.score > hits[kept - 1].score) {
            hits[kept - 1] = candidate;
            qsort(hits, kept, sizeof(ShardHit), hit_cmp);
        }
    }

    double elapsed = loogal_now_ms() - start;
    fclose(f);

    puts("SHARD FIND");
    puts("==============================");
    printf("query:    %s\n", query_path);
    printf("tiles:    %llu\n", (unsigned long long)header.tile_count);
    printf("returned: %d\n", kept);
    printf("time:     %.3f ms\n\n", elapsed);

    for (int i = 0; i < kept; i++) {
        char path[4096] = {0};
        load_path_for_id(hits[i].tile.image_id, path, sizeof(path));
        printf("%d. %.3f%%  %s\n", i + 1, hits[i].score * 100.0, path[0] ? path : "<unknown>");
        printf("   tile: x=%u y=%u w=%u h=%u dhash=%d ahash=%d\n",
               hits[i].tile.x, hits[i].tile.y, hits[i].tile.w, hits[i].tile.h,
               hits[i].dhash_distance, hits[i].ahash_distance);
    }

    free(hits);
    loogal_log("shard-find", "ok", query_path);
    return 0;
}
