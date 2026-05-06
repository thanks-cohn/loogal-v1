
#define _POSIX_C_SOURCE 200809L

#include "loogal/watch_run.h"
#include "loogal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define WATCH_FILE "data/watch/locations.tsv"

typedef struct {
    char path[4096];
    char schedule[32];
    char time_s[32];
    char date[32];
    char day[32];
    char enabled[16];
} WatchEntry;

static void trim_newline(char *s) {
    if (!s) return;
    s[strcspn(s, "\n")] = 0;
}

static void lower_ascii(char *s) {
    if (!s) return;
    for (; *s; s++) {
        if (*s >= 'A' && *s <= 'Z') *s = (char)(*s - 'A' + 'a');
    }
}

static int parse_tsv_line(char *line, WatchEntry *e) {
    memset(e, 0, sizeof(*e));

    char *cols[6] = {0};
    int c = 0;

    char *p = line;
    cols[c++] = p;

    while (*p && c < 6) {
        if (*p == '\t') {
            *p = 0;
            cols[c++] = p + 1;
        }
        p++;
    }

    if (cols[0]) snprintf(e->path, sizeof(e->path), "%.*s", (int)sizeof(e->path) - 1, cols[0]);
    if (cols[1]) snprintf(e->schedule, sizeof(e->schedule), "%s", cols[1]);
    if (cols[2]) snprintf(e->time_s, sizeof(e->time_s), "%s", cols[2]);
    if (cols[3]) snprintf(e->date, sizeof(e->date), "%s", cols[3]);
    if (cols[4]) snprintf(e->day, sizeof(e->day), "%s", cols[4]);
    if (cols[5]) snprintf(e->enabled, sizeof(e->enabled), "%s", cols[5]);

    return e->path[0] && e->schedule[0];
}

static const char *weekday3(int wday) {
    switch (wday) {
        case 0: return "sun";
        case 1: return "mon";
        case 2: return "tue";
        case 3: return "wed";
        case 4: return "thu";
        case 5: return "fri";
        case 6: return "sat";
        default: return "";
    }
}

static void current_hhmm(char *out, size_t out_sz, const struct tm *tmv) {
    snprintf(out, out_sz, "%02d:%02d", tmv->tm_hour, tmv->tm_min);
}

static void current_mmdd(char *out, size_t out_sz, const struct tm *tmv) {
    snprintf(out, out_sz, "%02d-%02d", tmv->tm_mon + 1, tmv->tm_mday);
}

static int due_now(const WatchEntry *e, const struct tm *tmv) {
    char sched[32];
    char day[32];
    char now_time[16];
    char now_date[16];

    snprintf(sched, sizeof(sched), "%s", e->schedule);
    snprintf(day, sizeof(day), "%s", e->day);
    lower_ascii(sched);
    lower_ascii(day);

    current_hhmm(now_time, sizeof(now_time), tmv);
    current_mmdd(now_date, sizeof(now_date), tmv);

    if (strcmp(e->enabled, "enabled") != 0) return 0;

    if (!strcmp(sched, "hourly")) {
        return 1;
    }

    if (!strcmp(sched, "daily")) {
        return e->time_s[0] && !strcmp(e->time_s, now_time);
    }

    if (!strcmp(sched, "weekly")) {
        return e->time_s[0] &&
               day[0] &&
               !strcmp(day, weekday3(tmv->tm_wday)) &&
               !strcmp(e->time_s, now_time);
    }

    if (!strcmp(sched, "yearly")) {
        return e->time_s[0] &&
               e->date[0] &&
               !strcmp(e->date, now_date) &&
               !strcmp(e->time_s, now_time);
    }

    return 0;
}

static int run_index_for_path(const char *path, int dry_run) {
    if (!path || !path[0]) {
        fprintf(stderr, "[loogal-watch-run:error] empty path\n");
        return 1;
    }

    if (dry_run) {
        printf("[loogal-watch-run:dry] loogal index %s\n", path);
        return 0;
    }

    printf("[loogal-watch-run] indexing: %s\n", path);
    fflush(stdout);

    char *argv[] = { "index", (char *)path };
    int rc = cmd_index(2, argv);

    if (rc == 0) {
        printf("[loogal-watch-run] OK: %s\n", path);
        return 0;
    }

    printf("[loogal-watch-run] FAILED rc=%d: %s\n", rc, path);
    return 1;
}

static void usage(void) {
    puts("LOOGAL WATCH-RUN");
    puts("");
    puts("Usage:");
    puts("  loogal watch-run");
    puts("  loogal watch-run --dry-run");
    puts("  loogal watch-run --all");
    puts("");
    puts("Behavior:");
    puts("  Reads data/watch/locations.tsv");
    puts("  Indexes enabled locations that are due right now.");
    puts("");
    puts("Schedules:");
    puts("  hourly              runs whenever watch-run is called");
    puts("  daily HH:MM         runs when local time matches HH:MM");
    puts("  weekly day HH:MM    runs when local day+time match");
    puts("  yearly MM-DD HH:MM  runs when local date+time match");
}

int loogal_cmd_watch_run(int argc, char **argv) {
    int dry_run = 0;
    int run_all = 0;

    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--dry-run")) {
            dry_run = 1;
        } else if (!strcmp(argv[i], "--all")) {
            run_all = 1;
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "[loogal-watch-run:error] unknown arg: %s\n", argv[i]);
            return 1;
        }
    }

    FILE *f = fopen(WATCH_FILE, "r");
    if (!f) {
        fprintf(stderr, "[loogal-watch-run:error] missing %s\n", WATCH_FILE);
        fprintf(stderr, "Run: loogal watch-list\n");
        return 1;
    }

    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);

    char now_time[16];
    char now_date[16];
    current_hhmm(now_time, sizeof(now_time), &tmv);
    current_mmdd(now_date, sizeof(now_date), &tmv);

    printf("[loogal-watch-run] now=%s %s %s\n",
           weekday3(tmv.tm_wday), now_date, now_time);

    char line[8192];
    int line_no = 0;
    int due_count = 0;
    int fail_count = 0;

    while (fgets(line, sizeof(line), f)) {
        line_no++;
        trim_newline(line);

        if (line_no == 1) continue;
        if (!line[0]) continue;

        WatchEntry e;
        if (!parse_tsv_line(line, &e)) continue;

        int due = run_all || due_now(&e, &tmv);

        if (!due) {
            printf("[loogal-watch-run] skip: %s (%s %s %s %s %s)\n",
                   e.path, e.schedule, e.time_s, e.date, e.day, e.enabled);
            continue;
        }

        due_count++;
        fail_count += run_index_for_path(e.path, dry_run);
    }

    fclose(f);

    printf("[loogal-watch-run] due=%d failed=%d\n", due_count, fail_count);
    return fail_count ? 1 : 0;
}
