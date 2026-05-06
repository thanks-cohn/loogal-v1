
#define _POSIX_C_SOURCE 200809L

#include "loogal/watch_config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#define WATCH_FILE "data/watch/locations.tsv"

typedef struct {
    char path[PATH_MAX];
    char schedule[32];
    char time[32];
    char date[32];
    char day[32];
    char enabled[16];
} WatchEntry;

static int ensure_watch_file(void) {
    if (mkdir("data", 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "[loogal:watch_config_error] could not create data directory\n");
        return 1;
    }
    if (mkdir("data/watch", 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "[loogal:watch_config_error] could not create data/watch directory\n");
        return 1;
    }
    FILE *f = fopen(WATCH_FILE, "r");
    if (f) {
        fclose(f);
        return 0;
    }

    f = fopen(WATCH_FILE, "w");
    if (!f) return -1;

    const char *home = getenv("HOME");
    if (!home) home = "";

    fprintf(f, "path\tschedule\ttime\tdate\tday\tenabled\n");
    fprintf(f, "%s/Pictures\tdaily\t00:00\t\t\tenabled\n", home);
    fclose(f);
return 0;
}

static int normalize_path(const char *in, char *out, size_t out_sz) {
    if (!in || !out || out_sz == 0) return 0;

    if (in[0] == '~') {
        const char *home = getenv("HOME");
        if (!home) return 0;
        snprintf(out, out_sz, "%s%s", home, in + 1);
    } else {
        snprintf(out, out_sz, "%s", in);
    }

    /*
     * Keep path normalization simple for v0.
     * We expand ~/ but do not require the path to already exist.
     * This avoids realpath() portability headaches and lets users add
     * future/external mount locations before they are present.
     */
    return 1;
}

static int parse_schedule(int argc, char **argv, int start, WatchEntry *e) {
    snprintf(e->schedule, sizeof(e->schedule), "daily");
    snprintf(e->time, sizeof(e->time), "00:00");
    e->date[0] = 0;
    e->day[0] = 0;

    for (int i = start; i < argc; i++) {
        if (!strcmp(argv[i], "--hourly")) {
            snprintf(e->schedule, sizeof(e->schedule), "hourly");
            e->time[0] = 0;
        } else if (!strcmp(argv[i], "--daily")) {
            snprintf(e->schedule, sizeof(e->schedule), "daily");
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                snprintf(e->time, sizeof(e->time), "%s", argv[++i]);
            } else {
                snprintf(e->time, sizeof(e->time), "00:00");
            }
        } else if (!strcmp(argv[i], "--weekly")) {
            if (i + 2 >= argc) {
                fprintf(stderr, "[loogal-watch:error] --weekly needs: <day> <HH:MM>\n");
                return 0;
            }
            snprintf(e->schedule, sizeof(e->schedule), "weekly");
            snprintf(e->day, sizeof(e->day), "%s", argv[++i]);
            snprintf(e->time, sizeof(e->time), "%s", argv[++i]);
        } else if (!strcmp(argv[i], "--yearly")) {
            if (i + 2 >= argc) {
                fprintf(stderr, "[loogal-watch:error] --yearly needs: <MM-DD> <HH:MM>\n");
                return 0;
            }
            snprintf(e->schedule, sizeof(e->schedule), "yearly");
            snprintf(e->date, sizeof(e->date), "%s", argv[++i]);
            snprintf(e->time, sizeof(e->time), "%s", argv[++i]);
        } else {
            fprintf(stderr, "[loogal-watch:error] unknown schedule arg: %s\n", argv[i]);
            return 0;
        }
    }

    snprintf(e->enabled, sizeof(e->enabled), "enabled");
    return 1;
}

static void print_entry(const WatchEntry *e) {
    printf("%-40s %-8s %-8s %-8s %-8s %s\n",
           e->path, e->schedule, e->time, e->date, e->day, e->enabled);
}

int loogal_cmd_watch_list(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (ensure_watch_file() != 0) {
return 1;
}

    FILE *f = fopen(WATCH_FILE, "r");
    if (!f) {
        perror(WATCH_FILE);
        return 1;
    }

    char line[8192];
    int first = 1;

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;

        if (first) {
            printf("%-40s %-8s %-8s %-8s %-8s %s\n",
                   "PATH", "SCHEDULE", "TIME", "DATE", "DAY", "STATUS");
            first = 0;
            continue;
        }

        WatchEntry e = {0};
        char *cols[6] = {0};
        int c = 0;
        char *save = NULL;
        char *tok = strtok_r(line, "\t", &save);
        while (tok && c < 6) {
            cols[c++] = tok;
            tok = strtok_r(NULL, "\t", &save);
        }

        if (cols[0]) snprintf(e.path, sizeof(e.path), "%s", cols[0]);
        if (cols[1]) snprintf(e.schedule, sizeof(e.schedule), "%s", cols[1]);
        if (cols[2]) snprintf(e.time, sizeof(e.time), "%s", cols[2]);
        if (cols[3]) snprintf(e.date, sizeof(e.date), "%s", cols[3]);
        if (cols[4]) snprintf(e.day, sizeof(e.day), "%s", cols[4]);
        if (cols[5]) snprintf(e.enabled, sizeof(e.enabled), "%s", cols[5]);
        print_entry(&e);
    }

    fclose(f);
    return 0;
}

int loogal_cmd_watch_add(int argc, char **argv) {
    if (ensure_watch_file() != 0) {
return 1;
}

    if (argc < 3) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  loogal watch-add <path> --daily 00:00\n");
        fprintf(stderr, "  loogal watch-add <path> --hourly\n");
        fprintf(stderr, "  loogal watch-add <path> --weekly sun 03:00\n");
        fprintf(stderr, "  loogal watch-add <path> --yearly 05-23 00:00\n");
        return 1;
    }

    WatchEntry e = {0};
    if (!normalize_path(argv[2], e.path, sizeof(e.path))) {
        fprintf(stderr, "[loogal-watch:error] failed to normalize path\n");
        return 1;
    }

    if (!parse_schedule(argc, argv, 3, &e)) return 1;

    FILE *in = fopen(WATCH_FILE, "r");
    FILE *out = fopen("data/watch/locations.tmp", "w");
    int updated = 0;

    if (!in || !out) {
        if (in) fclose(in);
        if (out) fclose(out);
        perror("watch config");
        return 1;
    }

    char line[8192];
    int first = 1;
    while (fgets(line, sizeof(line), in)) {
        char original[8192];
        snprintf(original, sizeof(original), "%s", line);

        if (first) {
            fputs(original, out);
            first = 0;
            continue;
        }

        char work[8192];
        snprintf(work, sizeof(work), "%s", line);
        work[strcspn(work, "\n")] = 0;

        char existing_path[PATH_MAX] = {0};
        char *tab = strchr(work, '\t');
        if (tab) *tab = 0;
        snprintf(existing_path, sizeof(existing_path), "%.*s", (int)sizeof(existing_path) - 1, work);

        if (!strcmp(existing_path, e.path)) {
            fprintf(out, "%s\t%s\t%s\t%s\t%s\t%s\n",
                    e.path, e.schedule, e.time, e.date, e.day, e.enabled);
            updated = 1;
        } else {
            fputs(original, out);
        }
    }

    if (!updated) {
        fprintf(out, "%s\t%s\t%s\t%s\t%s\t%s\n",
                e.path, e.schedule, e.time, e.date, e.day, e.enabled);
    }

    fclose(in);
    fclose(out);
    rename("data/watch/locations.tmp", WATCH_FILE);

    printf("[loogal-watch] added: %s %s %s %s %s %s\n",
           e.path, e.schedule, e.time, e.date, e.day, e.enabled);

    return 0;
}

static int rewrite_status(const char *target, const char *new_status, int remove_it) {
    if (ensure_watch_file() != 0) {
return 1;
}

    char norm[PATH_MAX];
    if (!normalize_path(target, norm, sizeof(norm))) return 1;

    FILE *in = fopen(WATCH_FILE, "r");
    FILE *out = fopen("data/watch/locations.tmp", "w");
    if (!in || !out) {
        perror("watch config");
        if (in) fclose(in);
        if (out) fclose(out);
        return 1;
    }

    char line[8192];
    int changed = 0;
    int first = 1;

    while (fgets(line, sizeof(line), in)) {
        if (first) {
            fputs(line, out);
            first = 0;
            continue;
        }

        char original[8192];
        snprintf(original, sizeof(original), "%s", line);
        line[strcspn(line, "\n")] = 0;

        WatchEntry e = {0};
        sscanf(line, "%4095[^\t]\t%31[^\t]\t%31[^\t]\t%31[^\t]\t%31[^\t]\t%15s",
               e.path, e.schedule, e.time, e.date, e.day, e.enabled);

        if (!strcmp(e.path, norm)) {
            changed = 1;
            if (remove_it) continue;
            snprintf(e.enabled, sizeof(e.enabled), "%s", new_status);
            fprintf(out, "%s\t%s\t%s\t%s\t%s\t%s\n",
                    e.path, e.schedule, e.time, e.date, e.day, e.enabled);
        } else {
            fputs(original, out);
        }
    }

    fclose(in);
    fclose(out);

    rename("data/watch/locations.tmp", WATCH_FILE);

    if (!changed) {
        fprintf(stderr, "[loogal-watch] no matching path: %s\n", norm);
        return 1;
    }

    return 0;
}

int loogal_cmd_watch_remove(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: loogal watch-remove <path>\n");
        return 1;
    }
    return rewrite_status(argv[2], "", 1);
}

int loogal_cmd_watch_enable(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: loogal watch-enable <path>\n");
        return 1;
    }
    return rewrite_status(argv[2], "enabled", 0);
}

int loogal_cmd_watch_disable(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: loogal watch-disable <path>\n");
        return 1;
    }
    return rewrite_status(argv[2], "disabled", 0);
}
