#include "verify.h"
#include "jsonout.h"
#include "loogal.h"
#include "loogal/platform.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long count_lines(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    long lines = 0;
    int c = 0;
    int last = '\n';

    while ((c = fgetc(f)) != EOF) {
        if (c == '\n') lines++;
        last = c;
    }

    if (last != '\n') lines++;

    fclose(f);
    return lines;
}

static long verify_file_size_bytes_local(const char *path) {
uint64_t size = 0;

if (loogal_platform_file_size(path, &size) != 0) {
return -1;
}

if (size > (uint64_t)LONG_MAX) {
return -1;
}

return (long)size;
}

static int extract_ull_field(const char *line, const char *key, unsigned long long *out) {
    char needle[128];
    snprintf(needle, sizeof(needle), "\"%s\":", key);

    const char *p = strstr(line, needle);
    if (!p) return 0;

    p += strlen(needle);

    while (*p == ' ' || *p == '\t') p++;

    char *end = NULL;
    unsigned long long value = strtoull(p, &end, 10);

    if (end == p) return 0;

    *out = value;
    return 1;
}

static int id_exists(const unsigned long long *ids, size_t count, unsigned long long id) {
    for (size_t i = 0; i < count; i++) {
        if (ids[i] == id) return 1;
    }
    return 0;
}

static int load_identity_ids(unsigned long long **out_ids, size_t *out_count) {
    *out_ids = NULL;
    *out_count = 0;

    FILE *f = fopen(LOOGAL_IDENTITIES_PATH, "r");
    if (!f) return -1;

    size_t cap = 0;
    char line[8192];

    while (fgets(line, sizeof(line), f)) {
        unsigned long long id = 0;
        if (!extract_ull_field(line, "id", &id)) continue;

        if (*out_count == cap) {
            size_t next_cap = cap ? cap * 2 : 64;
            unsigned long long *next = realloc(*out_ids, next_cap * sizeof(unsigned long long));
            if (!next) {
                fclose(f);
                free(*out_ids);
                *out_ids = NULL;
                *out_count = 0;
                return -1;
            }

            *out_ids = next;
            cap = next_cap;
        }

        (*out_ids)[(*out_count)++] = id;
    }

    fclose(f);
    return 0;
}

static long count_orphan_locations(const unsigned long long *ids, size_t id_count) {
    FILE *f = fopen(LOOGAL_LOCATIONS_PATH, "r");
    if (!f) return -1;

    long orphans = 0;
    char line[8192];

    while (fgets(line, sizeof(line), f)) {
        unsigned long long identity_id = 0;

        if (!extract_ull_field(line, "identity_id", &identity_id)) {
            orphans++;
            continue;
        }

        if (!id_exists(ids, id_count, identity_id)) {
            orphans++;
        }
    }

    fclose(f);
    return orphans;
}

static const char *ok_word(int ok) {
    return ok ? "ok" : "missing";
}

int cmd_verify(int argc, char **argv) {
    int as_json = loogal_has_flag(argc, argv, "--json");

    long identities = count_lines(LOOGAL_IDENTITIES_PATH);
    long locations = count_lines(LOOGAL_LOCATIONS_PATH);
    long events = count_lines(LOOGAL_EVENTS_PATH);
    long records = count_lines(LOOGAL_RECORDS_PATH);

    long bin_size = verify_file_size_bytes_local(LOOGAL_BIN_PATH);
    long expected_bin = -1;

    LoogalRecord *binary_records = NULL;
    size_t active_records = 0;
    int binary_read_ok = (read_index_records(&binary_records, &active_records) == 0);

    if (records >= 0) {
        expected_bin = 28 + records * (long)sizeof(LoogalRecord);
    }

    int identities_ok = identities >= 0;
    int locations_ok = locations >= 0;
    int events_ok = events >= 0;
    int records_ok = records >= 0;
    int bin_exists_ok = bin_size >= 0;
    int bin_size_ok = (records >= 0 && bin_size >= 0 && bin_size == expected_bin);
    int record_binary_count_ok = (records >= 0 && binary_read_ok && (long)active_records == records);
    int location_record_parity_ok = (locations >= 0 && records >= 0 && locations == records);

    unsigned long long *identity_ids = NULL;
    size_t identity_id_count = 0;
    long orphan_locations = -1;

    if (load_identity_ids(&identity_ids, &identity_id_count) == 0) {
        orphan_locations = count_orphan_locations(identity_ids, identity_id_count);
    }

    int orphan_ok = orphan_locations == 0;

    long duplicate_refs = 0;
    if (locations > 0 && identities > 0) {
        duplicate_refs = locations - identities;
        if (duplicate_refs < 0) duplicate_refs = 0;
    }

    int ok = 1;

    if (!identities_ok) ok = 0;
    if (!locations_ok) ok = 0;
    if (!events_ok) ok = 0;
    if (!records_ok) ok = 0;
    if (!bin_exists_ok) ok = 0;
    if (!bin_size_ok) ok = 0;
    if (!binary_read_ok) ok = 0;
    if (!record_binary_count_ok) ok = 0;
    if (!location_record_parity_ok) ok = 0;
    if (!orphan_ok) ok = 0;

    if (as_json) {
        puts("{");
        printf("  ");
        loogal_json_kv_string(stdout, "status", ok ? "ok" : "error", 1);
        printf("  ");
        loogal_json_kv_int(stdout, "visual_identities", identities_ok ? identities : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "known_locations", locations_ok ? locations : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "events_recorded", events_ok ? events : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "active_records", binary_read_ok ? (long long)active_records : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "duplicate_references", duplicate_refs, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "orphan_locations", orphan_locations >= 0 ? orphan_locations : -1, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "records_jsonl_count", records_ok ? records : -1, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "binary_size_bytes", bin_exists_ok ? bin_size : -1, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "expected_binary_size_bytes", expected_bin, 1);
        printf("  ");
        loogal_json_kv_string(stdout, "identities_jsonl", ok_word(identities_ok), 1);
        printf("  ");
        loogal_json_kv_string(stdout, "locations_jsonl", ok_word(locations_ok), 1);
        printf("  ");
        loogal_json_kv_string(stdout, "events_jsonl", ok_word(events_ok), 1);
        printf("  ");
        loogal_json_kv_string(stdout, "records_jsonl", ok_word(records_ok), 1);
        printf("  ");
        loogal_json_kv_string(stdout, "binary_index", binary_read_ok ? "ok" : "error", 0);
        puts("}");
    } else {
        puts("LOOGAL VERIFY");
        puts("────────────────────────");
        puts("");

        puts("Truth Files");
        if (identities_ok)
            printf("[OK] identities.jsonl lines : %ld\n", identities);
        else
            puts("[ERR] missing data/identities.jsonl");

        if (locations_ok)
            printf("[OK] locations.jsonl lines  : %ld\n", locations);
        else
            puts("[ERR] missing data/locations.jsonl");

        if (events_ok)
            printf("[OK] events.jsonl lines     : %ld\n", events);
        else
            puts("[ERR] missing data/events.jsonl");

        if (records_ok)
            printf("[OK] records.jsonl lines    : %ld\n", records);
        else
            puts("[ERR] missing data/records.jsonl");

        puts("");
        puts("Hot Index");

        if (bin_exists_ok)
            printf("[OK] loogal.bin bytes       : %ld\n", bin_size);
        else
            puts("[ERR] missing data/loogal.bin");

        if (records_ok && bin_exists_ok) {
            printf("[INFO] expected bin bytes   : %ld\n", expected_bin);

            if (bin_size_ok)
                puts("[OK] binary size parity     : ok");
            else
                puts("[ERR] binary size parity    : mismatch");
        }

        if (binary_read_ok)
            printf("[OK] binary index readable  : yes\n");
        else
            puts("[ERR] binary index readable : no");

        if (binary_read_ok)
            printf("[OK] active records         : %zu\n", active_records);

        puts("");
        puts("Integrity");

        if (orphan_locations >= 0) {
            if (orphan_ok)
                puts("[OK] orphan locations       : 0");
            else
                printf("[ERR] orphan locations      : %ld\n", orphan_locations);
        } else {
            puts("[ERR] orphan locations      : unknown");
        }

        if (location_record_parity_ok)
            printf("[OK] location/record parity : %ld == %ld\n", locations, records);
        else if (locations >= 0 && records >= 0)
            printf("[ERR] location/record parity: %ld != %ld\n", locations, records);
        else
            puts("[ERR] location/record parity: unavailable");

        if (record_binary_count_ok)
            printf("[OK] record/binary parity   : %ld == %zu\n", records, active_records);
        else if (records >= 0 && binary_read_ok)
            printf("[ERR] record/binary parity  : %ld != %zu\n", records, active_records);
        else
            puts("[ERR] record/binary parity  : unavailable");

        printf("[OK] duplicate references   : %ld\n", duplicate_refs);

        puts("");
        printf("System integrity: %s\n", ok ? "OK" : "ERROR");
    }

    loogal_log("verify", ok ? "ok" : "error", ok ? "immortal memory verified" : "immortal memory verification failed");

    free(identity_ids);
    free(binary_records);

    return ok ? 0 : 1;
}
