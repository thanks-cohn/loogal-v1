#include "loogal/platform.h"
#include <stdio.h>

static int exists(const char *path) {
return loogal_platform_file_exists(path);
}

int loogal_doctor(void) {
    printf("LOOGAL DOCTOR\n");
    printf("────────────────────────\n");

    int ok = 1;

    if (exists("data/loogal.bin")) {
        printf("[OK] binary index present\n");
    } else {
        printf("[ERR] missing data/loogal.bin\n");
        ok = 0;
    }

    if (exists("data/records.jsonl")) {
        printf("[OK] truth records present\n");
    } else {
        printf("[ERR] missing data/records.jsonl\n");
        ok = 0;
    }

    if (exists("data/logs/loogal.jsonl")) {
        printf("[OK] logs active\n");
    } else {
        printf("[WARN] logs not found (will be created on next run)\n");
    }

    printf("\n");

    if (ok) {
        printf("System status: READY\n");
    } else {
        printf("System status: INCOMPLETE\n");
    }

    return 0;
}
