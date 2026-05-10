#include "loogal.h"
#include "shard_index.h"
#include "timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOGAL_SHARD_INDEX_PATH "data/shards.bin"
#define LOOGAL_SHARD_MAP_PATH   "data/shards.paths"

static const int SHARD_SCALES[] = {64, 96, 128, 192};
static const int SHARD_SCALE_COUNT = 4;

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

static int emit_tile(FILE *out, uint32_t image_id, const char *path, int x, int y, int w, int h) {
    uint64_t dhash = 0;
    uint64_t ahash = 0;

    if (compute_region_hashes(path, x, y, w, h, &dhash, &ahash) != 0) {
        return 0;
    }

    ShardDiskTile t;
    memset(&t, 0, sizeof(t));
    t.dhash = dhash;
    t.ahash = ahash;
    t.image_id = image_id;
    t.x = (uint32_t)x;
    t.y = (uint32_t)y;
    t.w = (uint32_t)w;
    t.h = (uint32_t)h;

    return fwrite(&t, sizeof(t), 1, out) == 1 ? 1 : 0;
}

static uint64_t emit_tiles_for_record(FILE *out, const LoogalRecord *rec, uint32_t image_id) {
    if (!rec || rec->width <= 0 || rec->height <= 0 || !rec->path[0]) return 0;

    uint64_t written = 0;

    for (int si = 0; si < SHARD_SCALE_COUNT; si++) {
        int tile = SHARD_SCALES[si];
        int stride = tile / 2;
        if (stride < 1) stride = 1;

        if (rec->width < tile || rec->height < tile) continue;

        for (int y = 0; y <= rec->height - tile; y += stride) {
            for (int x = 0; x <= rec->width - tile; x += stride) {
                written += (uint64_t)emit_tile(out, image_id, rec->path, x, y, tile, tile);
            }
        }

        int right_x = rec->width - tile;
        int bottom_y = rec->height - tile;

        if (right_x > 0) {
            for (int y = 0; y <= rec->height - tile; y += stride) {
                written += (uint64_t)emit_tile(out, image_id, rec->path, right_x, y, tile, tile);
            }
        }

        if (bottom_y > 0) {
            for (int x = 0; x <= rec->width - tile; x += stride) {
                written += (uint64_t)emit_tile(out, image_id, rec->path, x, bottom_y, tile, tile);
            }
        }

        if (right_x > 0 && bottom_y > 0) {
            written += (uint64_t)emit_tile(out, image_id, rec->path, right_x, bottom_y, tile, tile);
        }
    }

    if (written == 0) {
        int w = rec->width > 0 ? rec->width : LOOGAL_SHARD_TILE_SIZE;
        int h = rec->height > 0 ? rec->height : LOOGAL_SHARD_TILE_SIZE;
        written += (uint64_t)emit_tile(out, image_id, rec->path, 0, 0, w, h);
    }

    return written;
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
    header.version = 2;
    header.tile_size = LOOGAL_SHARD_TILE_SIZE;
    header.stride = LOOGAL_SHARD_STRIDE;
    header.tile_count = 0;

    fwrite(&header, sizeof(header), 1, out);

    double start = loogal_now_ms();
    uint64_t images_with_tiles = 0;

    for (size_t i = 0; i < record_count; i++) {
        write_path_map(map, (uint32_t)i, records[i].path);
        uint64_t n = emit_tiles_for_record(out, &records[i], (uint32_t)i);
        header.tile_count += n;
        if (n > 0) images_with_tiles++;
    }

    fseek(out, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, out);

    double elapsed = loogal_now_ms() - start;

    fclose(map);
    fclose(out);
    free(records);

    printf("SHARD INDEX BUILT\n");
    printf("mode:     overlapping region witnesses\n");
    printf("images:   %llu\n", (unsigned long long)images_with_tiles);
    printf("tiles:    %llu\n", (unsigned long long)header.tile_count);
    printf("scales:   64,96,128,192\n");
    printf("stride:   half tile size\n");
    printf("time:     %.3f ms\n", elapsed);
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
        loogal_die("shard-find", "usage: locus-shard find <crop.png> [--limit N] [--min N]");
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
        loogal_die("shard-find", "missing shard index; run locus-shard index after loogal index");
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
    int skipped_self = 0;

    for (uint64_t i = 0; i < header.tile_count; i++) {
        ShardDiskTile t;
        char path[4096] = {0};
        if (fread(&t, sizeof(t), 1, f) != 1) break;
        load_path_for_id(t.image_id, path, sizeof(path));
        if (path[0] && strcmp(path, query_path) == 0) {
            skipped_self++;
            continue;
        }

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
    printf("query:        %s\n", query_path);
    printf("tiles:        %llu\n", (unsigned long long)header.tile_count);
    printf("returned:     %d\n", kept);
    printf("skipped_self: %d\n", skipped_self);
    printf("time:         %.3f ms\n\n", elapsed);

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
