#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "jsonout.h"
#include "similar.h"
#include "session.h"
#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *path;
    const char *place;
    const char *min_percent;
    const char *limit;
    const char *offset;
    int memory;
    int as_json;
    int dry_run;
    int push_history;
} SimilarArgs;

static void similar_usage(void) {
    puts("LOOGAL SIMILAR — create a new search session from an image path");
    puts("");
    puts("Commands:");
    puts("  loogal similar --path <image> --place <dir> [--min N] [--limit N] [--json]");
    puts("  loogal similar --path <image> --memory [--limit N] [--json]");
    puts("  loogal similar --path <image> --place <dir> --push-history --json");
    puts("");
    puts("Purpose:");
    puts("  This is the future loogal-window Search Similar button.");
    puts("  It now uses direct C services instead of spawning ./loogal.");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return NULL;
}

static int has_flag_local(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) return 1;
    }
    return 0;
}

static int parse_args(int argc, char **argv, SimilarArgs *a) {
    memset(a, 0, sizeof(*a));

    a->path = arg_value(argc, argv, "--path");
    if (!a->path) a->path = arg_value(argc, argv, "--query");
    if (!a->path && argc >= 1 && argv[0][0] != '-') a->path = argv[0];

    a->place = arg_value(argc, argv, "--place");
    a->min_percent = arg_value(argc, argv, "--min");
    a->limit = arg_value(argc, argv, "--limit");
    a->offset = arg_value(argc, argv, "--offset");
    a->memory = has_flag_local(argc, argv, "--memory");
    a->as_json = has_flag_local(argc, argv, "--json");
    a->dry_run = has_flag_local(argc, argv, "--dry-run");
    a->push_history = has_flag_local(argc, argv, "--push-history");

    if (!a->min_percent) a->min_percent = "60";
    if (!a->limit) a->limit = "50";
    if (!a->offset) a->offset = "0";

    if (!a->path) return -1;
    if (!a->memory && !a->place) return -1;

    return 0;
}

static long parse_long_or_default(const char *s, long fallback) {
    if (!s || !*s) return fallback;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0') return fallback;
    return v;
}

int cmd_similar(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        similar_usage();
        return 0;
    }

    SimilarArgs a;
    if (parse_args(argc, argv, &a) != 0) {
        similar_usage();
        LOOGAL_ERROR(LOOGAL_ERR_CMD_MISSING_ARGUMENT, "similar", "parse_args", NULL, "missing --path and search scope");
        return 1;
    }

    if (a.dry_run) {
        if (a.as_json) {
            puts("{");
            printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
            printf("  "); loogal_json_kv_string(stdout, "type", "similar.dry_run", 1);
            printf("  "); loogal_json_kv_string(stdout, "path", a.path, 1);
            printf("  "); loogal_json_kv_string(stdout, "scope", a.memory ? "memory" : a.place, 1);
            printf("  "); loogal_json_kv_string(stdout, "execution", "direct.c", 0);
            puts("}");
        } else {
            printf("[loogal:similar_dry_run] direct.c path=%s scope=%s\n", a.path, a.memory ? "memory" : a.place);
        }

        LOOGAL_INFO("similar", "dry_run", a.path, "dry-run similar request");
        return 0;
    }

    LoogalSessionCreateResult session;
    if (loogal_session_create_direct(
            a.path,
            a.place,
            a.memory,
            a.min_percent,
            a.limit,
            a.offset,
            &session
        ) != 0) {
        fprintf(stderr, "[loogal:similar_error] session create failed through direct C service\n");
        LOOGAL_ERROR(LOOGAL_ERR_CMD_OPERATION_FAILED, "similar", "session_create_direct", a.path, "session create failed");
        return 1;
    }

    int history_pushed = 0;

    if (a.push_history) {
        long offset = parse_long_or_default(a.offset, 0);

        if (loogal_history_push_direct(a.path, session.id, offset, 1) != 0) {
            fprintf(stderr, "[loogal:similar_error] history push failed through direct C service\n");
            LOOGAL_ERROR(LOOGAL_ERR_CMD_OPERATION_FAILED, "similar", "history_push_direct", a.path, "history push failed");
            return 1;
        }

        history_pushed = 1;
    }

    LOOGAL_INFO("similar", "created", a.path, "created similar session through direct C services");

    if (a.as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "similar.created", 1);
        printf("  "); loogal_json_kv_string(stdout, "path", a.path, 1);
        printf("  "); loogal_json_kv_string(stdout, "scope", a.memory ? "memory" : a.place, 1);
        printf("  "); loogal_json_kv_string(stdout, "session_id", session.id, 1);
        printf("  "); loogal_json_kv_string(stdout, "meta", session.meta_path, 1);
        printf("  "); loogal_json_kv_string(stdout, "results", session.results_path, 1);
        printf("  \"bytes\": %zu,\n", session.bytes);
        printf("  \"history_pushed\": %s,\n", history_pushed ? "true" : "false");
        printf("  "); loogal_json_kv_string(stdout, "execution", "direct.c", 1);
        printf("  "); loogal_json_kv_string(stdout, "replay_command", session.replay_command, 0);
        puts("}");
    } else {
        printf("[loogal:similar_ok] session=%s path=%s\n", session.id, a.path);
        if (session.meta_path[0]) printf("  meta:    %s\n", session.meta_path);
        if (session.results_path[0]) printf("  results: %s\n", session.results_path);
        printf("  bytes:   %zu\n", session.bytes);
        if (history_pushed) printf("  history: pushed\n");
        printf("  engine:  direct.c\n");
    }

    return 0;
}
