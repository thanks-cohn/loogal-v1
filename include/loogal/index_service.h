#ifndef LOOGAL_INDEX_SERVICE_H
#define LOOGAL_INDEX_SERVICE_H

typedef struct {
    int hash_mode_v0;
    int dry_run;
    int changed;
    int known_unchanged;
    int decoded;
    int skipped;
} LoogalIndexOneResult;

int loogal_index_one_path(const char *path, int hash_mode_v0, int dry_run, LoogalIndexOneResult *out);

#endif
