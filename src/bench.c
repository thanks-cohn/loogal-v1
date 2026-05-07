#include "bench.h"
#include "jsonout.h"
#include "loogal.h"
#include "loogal/platform.h"
#include "timer.h"
#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#define LOOGAL_INDEX_HEADER_BYTES 28

static long bench_file_size(const char *path) {
uint64_t size = 0;

if (loogal_platform_file_size(path, &size) != 0) {
return -1;
}

if (size > (uint64_t)LONG_MAX) {
return -1;
}

return (long)size;
}

int cmd_bench(int argc, char **argv) {
    int as_json = loogal_has_flag(argc, argv, "--json");

    long bin_size = bench_file_size("data/loogal.bin");
    if (bin_size < 0) {
        puts("LOOGAL BENCH");
        puts("[ERR] missing data/loogal.bin");
        puts("Run:");
        puts("  loogal index <directories...>");
        loogal_log("bench", "error", "missing binary index");
        return 1;
    }

    if (bin_size < LOOGAL_INDEX_HEADER_BYTES) {
        puts("LOOGAL BENCH");
        puts("[ERR] binary index too small / corrupt");
        loogal_log("bench", "error", "binary index too small");
        return 1;
    }

    long payload = bin_size - LOOGAL_INDEX_HEADER_BYTES;
    long record_size = (long)sizeof(LoogalRecord);
    long records = payload / record_size;

    FILE *f = fopen("data/loogal.bin", "rb");
    if (!f) {
        puts("LOOGAL BENCH");
        puts("[ERR] could not open data/loogal.bin");
        loogal_log("bench", "error", "could not open binary index");
        return 1;
    }

    double start = loogal_now_ms();

    volatile uint64_t checksum = 0;
    unsigned char buf[8192];

    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        for (size_t i = 0; i < n; i++) {
            checksum += buf[i];
        }
    }

    double elapsed = loogal_now_ms() - start;
    fclose(f);

    double records_per_ms = elapsed > 0.0 ? ((double)records / elapsed) : 0.0;

    if (as_json) {
        puts("{");
        printf("  ");
        loogal_json_kv_string(stdout, "status", "ok", 1);
        printf("  ");
        loogal_json_kv_int(stdout, "binary_size_bytes", bin_size, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "index_header_bytes", LOOGAL_INDEX_HEADER_BYTES, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "record_size_bytes", record_size, 1);
        printf("  ");
        loogal_json_kv_int(stdout, "records_estimated", records, 1);
        printf("  ");
        printf("\"duration_ms\": %.3f,\n", elapsed);
        printf("  ");
        printf("\"records_per_ms\": %.3f,\n", records_per_ms);
        printf("  ");
        loogal_json_kv_int(stdout, "checksum", (long long)checksum, 0);
        puts("}");
    } else {
        puts("LOOGAL BENCH");
        puts("────────────────────────");
        printf("Binary index bytes : %ld\n", bin_size);
        printf("Header bytes       : %d\n", LOOGAL_INDEX_HEADER_BYTES);
        printf("Record size        : %ld\n", record_size);
        printf("Records estimated  : %ld\n", records);
        printf("Scan duration      : %.3f ms\n", elapsed);
        printf("Records / ms       : %.3f\n", records_per_ms);
        printf("Checksum           : %llu\n", (unsigned long long)checksum);
        puts("");
        puts("Interpretation:");
        puts("  This measures raw binary index scan readiness.");
        puts("  Search stays fast because it scans binary memory, not JSON.");
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "records=%ld duration_ms=%.3f records_per_ms=%.3f", records, elapsed, records_per_ms);
    loogal_log("bench", "ok", msg);

    return 0;
}
