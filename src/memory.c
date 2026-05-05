#define _XOPEN_SOURCE 700
#include "memory.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "LOOGAL ERROR: out of memory\n");
        exit(2);
    }
    return q;
}

static void ensure_identity_cap(LoogalMemory *m) {
    if (m->identity_count < m->identity_cap) return;
    m->identity_cap = m->identity_cap ? m->identity_cap * 2 : 1024;
    m->identities = xrealloc(m->identities, m->identity_cap * sizeof(LoogalIdentity));
}

static void ensure_location_cap(LoogalMemory *m) {
    if (m->location_count < m->location_cap) return;
    m->location_cap = m->location_cap ? m->location_cap * 2 : 1024;
    m->locations = xrealloc(m->locations, m->location_cap * sizeof(LoogalLocation));
}

static int extract_string_field(const char *line, const char *key, char *out, size_t out_sz) {
    char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\":\"", key);
    const char *p = strstr(line, pat);
    if (!p) return -1;
    p += strlen(pat);
    size_t j = 0;
    while (*p && *p != '"' && j + 1 < out_sz) {
        if (*p == '\\' && p[1]) p++;
        out[j++] = *p++;
    }
    out[j] = 0;
    return 0;
}

static int extract_u64_field(const char *line, const char *key, uint64_t *out) {
    char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\":", key);
    const char *p = strstr(line, pat);
    if (!p) return -1;
    p += strlen(pat);
    *out = strtoull(p, NULL, 10);
    return 0;
}

static int extract_i32_field(const char *line, const char *key, int32_t *out) {
    uint64_t v = 0;
    if (extract_u64_field(line, key, &v) != 0) return -1;
    *out = (int32_t)v;
    return 0;
}

static int extract_float_field(const char *line, const char *key, float *out) {
    char pat[64];
    snprintf(pat, sizeof(pat), "\"%s\":", key);
    const char *p = strstr(line, pat);
    if (!p) return -1;
    p += strlen(pat);
    *out = (float)strtod(p, NULL);
    return 0;
}

static LoogalIdentity *find_identity_by_sha(LoogalMemory *m, const char *sha256) {
    if (!sha256 || !sha256[0]) return NULL;
    for (size_t i = 0; i < m->identity_count; i++) {
        if (strcmp(m->identities[i].sha256, sha256) == 0) return &m->identities[i];
    }
    return NULL;
}

static LoogalLocation *find_location_by_path(LoogalMemory *m, const char *path) {
    for (size_t i = 0; i < m->location_count; i++) {
        if (strcmp(m->locations[i].path, path) == 0) return &m->locations[i];
    }
    return NULL;
}

static uint64_t next_identity_id(const LoogalMemory *m) {
    uint64_t max = 0;
    for (size_t i = 0; i < m->identity_count; i++) if (m->identities[i].id > max) max = m->identities[i].id;
    return max + 1;
}

static uint64_t next_location_id(const LoogalMemory *m) {
    uint64_t max = 0;
    for (size_t i = 0; i < m->location_count; i++) if (m->locations[i].id > max) max = m->locations[i].id;
    return max + 1;
}

static void recompute_location_counts(LoogalMemory *m) {
    for (size_t i = 0; i < m->identity_count; i++) m->identities[i].location_count = 0;
    for (size_t j = 0; j < m->location_count; j++) {
        if (m->locations[j].status != LOOGAL_LOCATION_ACTIVE) continue;
        for (size_t i = 0; i < m->identity_count; i++) {
            if (m->identities[i].id == m->locations[j].identity_id) {
                m->identities[i].location_count++;
                break;
            }
        }
    }
}

void loogal_memory_init(LoogalMemory *m) {
    memset(m, 0, sizeof(*m));
}

void loogal_memory_free(LoogalMemory *m) {
    free(m->identities);
    free(m->locations);
    memset(m, 0, sizeof(*m));
}

int loogal_memory_append_event(const char *type, const char *status, const char *path, uint64_t identity_id, const char *message) {
    ensure_dirs();
    FILE *f = fopen(LOOGAL_EVENTS_PATH, "a");
    if (!f) return -1;
    char *epath = json_escape(path ? path : "");
    char *emsg = json_escape(message ? message : "");
    fprintf(f, "{\"ts\":%ld,\"tool\":\"loogal\",\"type\":\"%s\",\"status\":\"%s\",\"identity_id\":%llu,\"path\":\"%s\",\"message\":\"%s\"}\n",
            now_unix(), type ? type : "event", status ? status : "ok",
            (unsigned long long)identity_id, epath ? epath : "", emsg ? emsg : "");
    free(epath);
    free(emsg);
    fclose(f);
    return 0;
}

int loogal_memory_load(LoogalMemory *m) {
    loogal_memory_init(m);
    ensure_dirs();

    FILE *f = fopen(LOOGAL_IDENTITIES_PATH, "r");
    if (f) {
        char line[4096];
        while (fgets(line, sizeof(line), f)) {
            LoogalIdentity id;
            memset(&id, 0, sizeof(id));
            if (extract_u64_field(line, "id", &id.id) != 0) continue;
            extract_string_field(line, "sha256", id.sha256, sizeof(id.sha256));
            extract_u64_field(line, "dhash", &id.dhash);
            extract_float_field(line, "aspect", &id.aspect);
            extract_u64_field(line, "best_file_size", &id.best_file_size);
            extract_i32_field(line, "width", &id.width);
            extract_i32_field(line, "height", &id.height);
            extract_u64_field(line, "location_count", &id.location_count);
            extract_u64_field(line, "first_seen_unix", &id.first_seen_unix);
            extract_u64_field(line, "last_seen_unix", &id.last_seen_unix);
            ensure_identity_cap(m);
            m->identities[m->identity_count++] = id;
        }
        fclose(f);
    }

    f = fopen(LOOGAL_LOCATIONS_PATH, "r");
    if (f) {
        char line[4096];
        while (fgets(line, sizeof(line), f)) {
            LoogalLocation loc;
            memset(&loc, 0, sizeof(loc));
            if (extract_u64_field(line, "id", &loc.id) != 0) continue;
            extract_u64_field(line, "identity_id", &loc.identity_id);
            extract_string_field(line, "path", loc.path, sizeof(loc.path));
            extract_string_field(line, "kind", loc.kind, sizeof(loc.kind));
            extract_u64_field(line, "file_size", &loc.file_size);
            extract_u64_field(line, "seen_unix", &loc.seen_unix);
            uint64_t status = LOOGAL_LOCATION_ACTIVE;
            extract_u64_field(line, "status", &status);
            loc.status = (uint32_t)status;
            ensure_location_cap(m);
            m->locations[m->location_count++] = loc;
        }
        fclose(f);
    }

    recompute_location_counts(m);
    return 0;
}

static int write_identities_tmp(const LoogalMemory *m, const char *tmp) {
    FILE *f = fopen(tmp, "w");
    if (!f) return -1;
    for (size_t i = 0; i < m->identity_count; i++) {
        const LoogalIdentity *id = &m->identities[i];
        fprintf(f, "{\"id\":%llu,\"type\":\"visual.identity\",\"sha256\":\"%s\",\"dhash\":%llu,\"aspect\":%.6f,\"best_file_size\":%llu,\"width\":%d,\"height\":%d,\"location_count\":%llu,\"first_seen_unix\":%llu,\"last_seen_unix\":%llu}\n",
                (unsigned long long)id->id, id->sha256,
                (unsigned long long)id->dhash, id->aspect,
                (unsigned long long)id->best_file_size, id->width, id->height,
                (unsigned long long)id->location_count,
                (unsigned long long)id->first_seen_unix,
                (unsigned long long)id->last_seen_unix);
    }
    fclose(f);
    return 0;
}

static int write_locations_tmp(const LoogalMemory *m, const char *tmp) {
    FILE *f = fopen(tmp, "w");
    if (!f) return -1;
    for (size_t i = 0; i < m->location_count; i++) {
        const LoogalLocation *loc = &m->locations[i];
        char *p = json_escape(loc->path);
        fprintf(f, "{\"id\":%llu,\"type\":\"visual.location\",\"identity_id\":%llu,\"path\":\"%s\",\"kind\":\"%s\",\"file_size\":%llu,\"seen_unix\":%llu,\"status\":%u}\n",
                (unsigned long long)loc->id,
                (unsigned long long)loc->identity_id,
                p ? p : "", loc->kind,
                (unsigned long long)loc->file_size,
                (unsigned long long)loc->seen_unix,
                loc->status);
        free(p);
    }
    fclose(f);
    return 0;
}

int loogal_memory_save(const LoogalMemory *m) {
    ensure_dirs();
    const char *itmp = "data/identities.jsonl.tmp";
    const char *ltmp = "data/locations.jsonl.tmp";
    if (write_identities_tmp(m, itmp) != 0) return -1;
    if (write_locations_tmp(m, ltmp) != 0) return -1;
    if (rename(itmp, LOOGAL_IDENTITIES_PATH) != 0) return -1;
    if (rename(ltmp, LOOGAL_LOCATIONS_PATH) != 0) return -1;
    return 0;
}

int loogal_memory_reset_files(void) {
    unlink(LOOGAL_IDENTITIES_PATH);
    unlink(LOOGAL_LOCATIONS_PATH);
    unlink(LOOGAL_EVENTS_PATH);
    unlink(LOOGAL_RECORDS_PATH);
    unlink(LOOGAL_BIN_PATH);
    loogal_memory_append_event("memory.fresh", "ok", "", 0, "fresh memory reset requested");
    return 0;
}

int loogal_memory_ingest_image(LoogalMemory *m, const LoogalImageInfo *info) {
    uint64_t ts = (uint64_t)now_unix();
    LoogalLocation *existing_path = find_location_by_path(m, info->path);
    LoogalIdentity *identity = find_identity_by_sha(m, info->sha256);

    if (!identity) {
        ensure_identity_cap(m);
        LoogalIdentity *id = &m->identities[m->identity_count++];
        memset(id, 0, sizeof(*id));
        id->id = next_identity_id(m);
        snprintf(id->sha256, sizeof(id->sha256), "%s", info->sha256);
        id->dhash = info->dhash;
        id->aspect = info->aspect;
        id->best_file_size = info->file_size;
        id->width = info->width;
        id->height = info->height;
        id->first_seen_unix = ts;
        id->last_seen_unix = ts;
        identity = id;
        m->seen_new_identities++;
        loogal_memory_append_event("identity.created", "ok", info->path, identity->id, "new visual identity");
    } else {
        identity->last_seen_unix = ts;
    }

    if (existing_path) {
        if (existing_path->identity_id == identity->id) {
            existing_path->seen_unix = ts;
            existing_path->file_size = info->file_size;
            existing_path->status = LOOGAL_LOCATION_ACTIVE;
            m->seen_known_locations++;
            loogal_memory_append_event("location.seen", "ok", info->path, identity->id, "known location refreshed");
            recompute_location_counts(m);
            return 0;
        }
        existing_path->identity_id = identity->id;
        existing_path->seen_unix = ts;
        existing_path->file_size = info->file_size;
        existing_path->status = LOOGAL_LOCATION_ACTIVE;
        m->seen_modified_paths++;
        loogal_memory_append_event("location.modified", "ok", info->path, identity->id, "path now points to different identity");
        recompute_location_counts(m);
        return 0;
    }

    ensure_location_cap(m);
    LoogalLocation *loc = &m->locations[m->location_count++];
    memset(loc, 0, sizeof(*loc));
    loc->id = next_location_id(m);
    loc->identity_id = identity->id;
    snprintf(loc->path, sizeof(loc->path), "%s", info->path);
    snprintf(loc->kind, sizeof(loc->kind), "%s", "file");
    loc->file_size = info->file_size;
    loc->seen_unix = ts;
    loc->status = LOOGAL_LOCATION_ACTIVE;

    m->seen_new_locations++;
    if (identity->location_count > 0) {
        m->seen_duplicate_locations++;
        loogal_memory_append_event("location.duplicate", "ok", info->path, identity->id, "same identity at additional location");
    } else {
        loogal_memory_append_event("location.created", "ok", info->path, identity->id, "first active location for identity");
    }
    recompute_location_counts(m);
    return 0;
}

int loogal_memory_build_records(const LoogalMemory *m, LoogalRecord **out_records, size_t *out_count) {
    *out_records = NULL;
    *out_count = 0;
    size_t active = 0;
    for (size_t i = 0; i < m->location_count; i++) if (m->locations[i].status == LOOGAL_LOCATION_ACTIVE) active++;
    LoogalRecord *records = calloc(active ? active : 1, sizeof(LoogalRecord));
    if (!records && active) return -1;

    size_t n = 0;
    for (size_t i = 0; i < m->location_count; i++) {
        const LoogalLocation *loc = &m->locations[i];
        if (loc->status != LOOGAL_LOCATION_ACTIVE) continue;
        const LoogalIdentity *id = NULL;
        for (size_t j = 0; j < m->identity_count; j++) {
            if (m->identities[j].id == loc->identity_id) { id = &m->identities[j]; break; }
        }
        if (!id) continue;
        LoogalRecord *r = &records[n];
        memset(r, 0, sizeof(*r));
        r->id = id->id;
        r->dhash = id->dhash;
        r->aspect = id->aspect;
        r->file_size = loc->file_size;
        r->width = id->width;
        r->height = id->height;
        snprintf(r->path, sizeof(r->path), "%s", loc->path);
        n++;
    }
    *out_records = records;
    *out_count = n;
    return 0;
}
