#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "jsonout.h"
#include "window_api.h"
#include "session.h"
#include "history.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WINDOW_API_CMD_MAX
#define WINDOW_API_CMD_MAX 8192
#endif

#ifndef WINDOW_API_OUTPUT_MAX
#define WINDOW_API_OUTPUT_MAX 1048576
#endif

static void window_api_usage(void) {
    puts("LOOGAL WINDOW-API — stable endpoints for the future GTK window");
    puts("");
    puts("Commands:");
    puts("  loogal window-api page --session <id> [--offset N] [--limit N] [--json]");
    puts("  loogal window-api current [--json]");
    puts("  loogal window-api similar --path <image> (--place <dir>|--memory) [--push-history] [--json]");
    puts("");
    puts("Purpose:");
    puts("  This is the window-facing API layer.");
    puts("  The future loogal-window calls this instead of guessing how sessions/history/similar fit together.");
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





static void print_json_string_field(const char *key, const char *val, int comma) {
    printf("\"");
    fputs(key, stdout);
    printf("\": ");
    loogal_json_string(stdout, val ? val : "");
    if (comma) putchar(',');
    putchar('\n');
}





static int cmd_window_api_page(int argc, char **argv) {
const char *session = arg_value(argc, argv, "--session");
const char *offset = arg_value(argc, argv, "--offset");
const char *limit = arg_value(argc, argv, "--limit");
int dry_run = has_flag_local(argc, argv, "--dry-run");

if (!session) {
    fprintf(stderr, "[loogal:window_api_error] missing --session\n");
    LOOGAL_ERROR(LOOGAL_ERR_CMD_MISSING_ARGUMENT,
                 "window_api",
                 "page",
                 NULL,
                 "missing --session");
    return 1;
}

if (!offset) offset = "0";
if (!limit) limit = "10";

if (dry_run) {
    puts("{");
    print_json_string_field("tool", "loogal", 1);
    print_json_string_field("type", "window_api.page.dry_run", 1);
    print_json_string_field("session", session, 1);
    print_json_string_field("offset", offset, 1);
    print_json_string_field("limit", limit, 1);
    print_json_string_field("execution", "direct.c", 0);
    puts("}");
    return 0;
}

char *session_argv[8];
int session_argc = 0;

session_argv[session_argc++] = "page";
session_argv[session_argc++] = (char *)session;
session_argv[session_argc++] = "--offset";
session_argv[session_argc++] = (char *)offset;
session_argv[session_argc++] = "--limit";
session_argv[session_argc++] = (char *)limit;
session_argv[session_argc++] = "--json";

int rc = cmd_session(session_argc, session_argv);

if (rc == 0) {
    LOOGAL_INFO("window_api",
                "page_direct",
                session,
                "window-api page served through direct session command");
} else {
    LOOGAL_ERROR(LOOGAL_ERR_CMD_OPERATION_FAILED,
                 "window_api",
                 "page_direct",
                 session,
                 "direct session page failed");
}

return rc;
}


static int cmd_window_api_current(int argc, char **argv) {
int dry_run = has_flag_local(argc, argv, "--dry-run");

if (dry_run) {
    puts("{");
    print_json_string_field("tool", "loogal", 1);
    print_json_string_field("type", "window_api.current.dry_run", 1);
    print_json_string_field("execution", "direct.c", 0);
    puts("}");
    return 0;
}

char *history_argv[2];
history_argv[0] = "current";
history_argv[1] = "--json";

int rc = cmd_history(2, history_argv);

if (rc == 0) {
    LOOGAL_INFO("window_api",
                "current_direct",
                NULL,
                "window-api current served through direct history command");
} else {
    LOOGAL_ERROR(LOOGAL_ERR_CMD_OPERATION_FAILED,
                 "window_api",
                 "current_direct",
                 NULL,
                 "direct history current failed");
}

return rc;
}


static int cmd_window_api_similar(int argc, char **argv) {
const char *path = arg_value(argc, argv, "--path");
const char *place = arg_value(argc, argv, "--place");
const char *limit = arg_value(argc, argv, "--limit");
const char *minp = arg_value(argc, argv, "--min");
const char *offset = arg_value(argc, argv, "--offset");

int memory = has_flag_local(argc, argv, "--memory");
int push_history = has_flag_local(argc, argv, "--push-history");
int as_json = has_flag_local(argc, argv, "--json");
int dry_run = has_flag_local(argc, argv, "--dry-run");

if (!path) {
    fprintf(stderr, "[loogal:window_api_error] missing --path\n");
    LOOGAL_ERROR(LOOGAL_ERR_CMD_MISSING_ARGUMENT,
                 "window_api",
                 "similar",
                 NULL,
                 "missing --path");
    return 1;
}

if (!place && !memory) {
    fprintf(stderr, "[loogal:window_api_error] provide --place <dir> or --memory\n");
    LOOGAL_ERROR(LOOGAL_ERR_CMD_MISSING_ARGUMENT,
                 "window_api",
                 "similar",
                 path,
                 "missing search scope");
    return 1;
}

if (!limit) limit = "50";
if (!minp) minp = "60";
if (!offset) offset = "0";

if (dry_run) {
    puts("{");
    print_json_string_field("tool", "loogal", 1);
    print_json_string_field("type", "window_api.similar.dry_run", 1);
    print_json_string_field("path", path, 1);
    print_json_string_field("scope", memory ? "memory" : place, 1);
    print_json_string_field("execution", "direct.c", 0);
    puts("}");
    return 0;
}

LoogalSessionCreateResult session;

if (loogal_session_create_direct(
        path,
        place,
        memory,
        minp,
        limit,
        offset,
        &session
    ) != 0) {

    fprintf(stderr, "[loogal:window_api_error] session creation failed\n");

    LOOGAL_ERROR(LOOGAL_ERR_CMD_OPERATION_FAILED,
                 "window_api",
                 "similar_create",
                 path,
                 "direct session create failed");

    return 1;
}

int history_pushed = 0;

if (push_history) {
    long off = strtol(offset, NULL, 10);

    if (loogal_history_push_direct(path, session.id, off, 1) != 0) {

        fprintf(stderr, "[loogal:window_api_error] history push failed\n");

        LOOGAL_ERROR(LOOGAL_ERR_CMD_OPERATION_FAILED,
                     "window_api",
                     "history_push",
                     path,
                     "direct history push failed");

        return 1;
    }

    history_pushed = 1;
}

LOOGAL_INFO("window_api",
            "similar_created",
            path,
            "window-api similar created through direct services");

if (as_json) {
    puts("{");
    print_json_string_field("tool", "loogal", 1);
    print_json_string_field("type", "window_api.similar.created", 1);
    print_json_string_field("path", path, 1);
    print_json_string_field("scope", memory ? "memory" : place, 1);
    print_json_string_field("session_id", session.id, 1);
    print_json_string_field("meta", session.meta_path, 1);
    print_json_string_field("results", session.results_path, 1);

    printf("\"bytes\": %zu,\n", session.bytes);
    printf("\"history_pushed\": %s,\n",
           history_pushed ? "true" : "false");

    print_json_string_field("execution", "direct.c", 1);
    print_json_string_field("replay_command",
                            session.replay_command,
                            0);

    puts("}");
} else {
    printf("[loogal:window_api_ok] session=%s path=%s\n",
           session.id,
           path);
}

return 0;
}

int cmd_window_api(int argc, char **argv) {
    if (argc <= 0 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        window_api_usage();
        return 0;
    }

    if (strcmp(argv[0], "page") == 0) return cmd_window_api_page(argc - 1, argv + 1);
    if (strcmp(argv[0], "current") == 0) return cmd_window_api_current(argc - 1, argv + 1);
    if (strcmp(argv[0], "similar") == 0) return cmd_window_api_similar(argc - 1, argv + 1);

    fprintf(stderr, "[loogal:window_api_error] unknown subcommand: %s\n", argv[0]);
    window_api_usage();
    return 1;
}
