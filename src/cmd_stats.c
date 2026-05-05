#include "loogal.h"
#include "jsonout.h"

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

static long count_locations_from_identities(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    long total = 0;
    char line[8192];

    while (fgets(line, sizeof(line), f)) {
        char *p = strstr(line, "\"location_count\":");
        if (!p) continue;

        p += strlen("\"location_count\":");
        total += strtol(p, NULL, 10);
    }

    fclose(f);
    return total;
}

static const char *status_word(int ok) {
    return ok ? "ok" : "missing";
}

int cmd_stats(int argc, char **argv) {
    int as_json = loogal_has_flag(argc, argv, "--json");

    (void)argc;
    (void)argv;

    LoogalRecord *records = NULL;
    size_t active_records = 0;
    int binary_ok = (read_index_records(&records, &active_records) == 0);

    size_t near_dupes = 0;
    if (binary_ok && records) {
        for (size_t i = 0; i < active_records; i++) {
            for (size_t j = i + 1; j < active_records; j++) {
                if (similarity_percent(records[i].dhash, records[j].dhash) >= 90) {
                    near_dupes++;
                    break;
                }
            }
        }
    }

    long identity_count = count_lines(LOOGAL_IDENTITIES_PATH);
    long location_count = count_lines(LOOGAL_LOCATIONS_PATH);
    long event_count = count_lines(LOOGAL_EVENTS_PATH);
    long record_lines = count_lines(LOOGAL_RECORDS_PATH);

    int identities_ok = identity_count >= 0;
    int locations_ok = location_count >= 0;
    int events_ok = event_count >= 0;
    int records_ok = record_lines >= 0;

    long identity_location_total = count_locations_from_identities(LOOGAL_IDENTITIES_PATH);
    long duplicate_refs = 0;

    if (location_count > 0 && identity_count > 0) {
        duplicate_refs = location_count - identity_count;
        if (duplicate_refs < 0) duplicate_refs = 0;
    }

    if (identity_location_total > 0 && identity_count > 0) {
        long from_identity = identity_location_total - identity_count;
        if (from_identity > duplicate_refs) duplicate_refs = from_identity;
    }

    if (as_json) {
        puts("{");
        printf("  ");
        loogal_json_kv_string(stdout, "status", "ok", 1);
        printf("  ");
        loogal_json_kv_int(stdout, "visual_identities", identities_ok ? identity_count : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "known_locations", locations_ok ? location_count : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "duplicate_references", duplicate_refs, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "events_recorded", events_ok ? event_count : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "active_search_records", binary_ok ? (long long)active_records : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "legacy_record_lines", records_ok ? record_lines : 0, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "redundant_near_duplicate_estimate", (long long)near_dupes, 1);
        printf("  ");
        loogal_json_kv_string(stdout, "identity_memory", status_word(identities_ok), 1);
        printf("  ");
        loogal_json_kv_string(stdout, "location_memory", status_word(locations_ok), 1);
        printf("  ");
        loogal_json_kv_string(stdout, "event_memory", status_word(events_ok), 1);
        printf("  ");
        loogal_json_kv_string(stdout, "binary_index", binary_ok ? "ok" : "missing", 0);
        puts("}");

        loogal_log("stats", "ok", "printed immortal status json");
        free(records);
        return 0;
    }

    puts("LOOGAL — VISUAL MEMORY STATUS");
    puts("────────────────────────────────────────");
    puts("");

    puts("Identity Memory");
    printf("  Visual identities          : %ld\n", identities_ok ? identity_count : 0);
    printf("  Known locations            : %ld\n", locations_ok ? location_count : 0);
    printf("  Duplicate references       : %ld\n", duplicate_refs);
    printf("  Events recorded            : %ld\n", events_ok ? event_count : 0);
    puts("");

    puts("Active Search");
    if (binary_ok) {
        printf("  Search records             : %zu\n", active_records);
        printf("  Near-duplicate estimate    : %zu\n", near_dupes);
        puts("  Binary index               : ok");
    } else {
        puts("  Search records             : 0");
        puts("  Near-duplicate estimate    : 0");
        puts("  Binary index               : missing");
    }
    puts("");

    puts("Truth Files");
    printf("  identities.jsonl           : %s\n", status_word(identities_ok));
    printf("  locations.jsonl            : %s\n", status_word(locations_ok));
    printf("  events.jsonl               : %s\n", status_word(events_ok));
    printf("  records.jsonl              : %s\n", status_word(records_ok));
    puts("");

    puts("Perceptual Model");
    puts("  Signature engine           : dHash v1 + sha256 exact identity");
    puts("  Comparison mode            : deterministic");
    puts("");

    puts("Index Engine");
    puts("  Storage model              : JSONL truth + packed binary hot index");
    puts("  Search execution           : binary-first");
    puts("  Recall readiness           : immediate");
    puts("");

    puts("Interpretation");
    puts("  Loogal now remembers visual identities separately from locations.");
    puts("  The same image can exist in multiple places without becoming multiple identities.");

    loogal_log("stats", "ok", "printed immortal status");
    free(records);
    return 0;
}
