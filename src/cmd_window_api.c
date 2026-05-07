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
#include <sys/wait.h>
#include <unistd.h>

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

static int shell_quote(const char *in, char *out, size_t out_sz) {
    if (!in || !out || out_sz < 3) return -1;
    size_t j = 0;
    out[j++] = '\'';
    for (size_t i = 0; in[i]; i++) {
        if (in[i] == '\'') {
            const char *esc = "'\\''";
            for (size_t k = 0; esc[k]; k++) {
                if (j + 1 >= out_sz) return -1;
                out[j++] = esc[k];
            }
        } else {
            if (j + 1 >= out_sz) return -1;
            out[j++] = in[i];
        }
    }
    if (j + 1 >= out_sz) return -1;
    out[j++] = '\'';
    out[j] = 0;
    return 0;
}

static int read_command_output(const char *cmd, char *out, size_t out_sz, int *exit_code) {
    if (!cmd || !out || out_sz == 0) return -1;
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    size_t used = 0;
    out[0] = 0;
    while (!feof(fp)) {
        if (used + 1 >= out_sz) {
            pclose(fp);
            return -2;
        }
        size_t n = fread(out + used, 1, out_sz - used - 1, fp);
        used += n;
        out[used] = 0;
        if (ferror(fp)) {
            pclose(fp);
            return -1;
        }
    }
    int st = pclose(fp);
    if (exit_code) {
        if (WIFEXITED(st)) *exit_code = WEXITSTATUS(st);
        else *exit_code = 127;
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

static int emit_dry_run(const char *op, const char *cmd) {
    puts("{");
    print_json_string_field("tool", "loogal", 1);
    print_json_string_field("type", "window_api.dry_run", 1);
    print_json_string_field("operation", op, 1);
    print_json_string_field("command", cmd, 1);
    printf("\"dry_run\": 1\n");
    puts("}");
    return 0;
}

static int run_passthrough_json(const char *op, const char *cmd, int as_json, int dry_run) {
    if (dry_run) return emit_dry_run(op, cmd);

    char *out = malloc(WINDOW_API_OUTPUT_MAX);
    if (!out) {
        fprintf(stderr, "[loogal:window_api_error] out of memory\n");
        return 1;
    }

    int code = 0;
    int rc = read_command_output(cmd, out, WINDOW_API_OUTPUT_MAX, &code);
    if (rc != 0 || code != 0) {
        if (as_json) {
            puts("{");
            print_json_string_field("tool", "loogal", 1);
            print_json_string_field("type", "window_api.error", 1);
            print_json_string_field("operation", op, 1);
            print_json_string_field("command", cmd, 1);
            printf("\"status\": \"error\",\n");
            printf("\"exit_code\": %d\n", code);
            puts("}");
        } else {
            fprintf(stderr, "[loogal:window_api_error] command failed — %s\n", cmd);
        }
        free(out);
        return 1;
    }

    fputs(out, stdout);
    size_t len = strlen(out);
    if (len == 0 || out[len - 1] != '\n') putchar('\n');
    free(out);
    return 0;
}

static int cmd_window_api_page(int argc, char **argv) {
    const char *session = arg_value(argc, argv, "--session");
    const char *offset = arg_value(argc, argv, "--offset");
    const char *limit = arg_value(argc, argv, "--limit");
    int as_json = has_flag_local(argc, argv, "--json");
    int dry_run = has_flag_local(argc, argv, "--dry-run");

    if (!session) {
        fprintf(stderr, "[loogal:window_api_error] missing --session\n");
        return 1;
    }
    if (!offset) offset = "0";
    if (!limit) limit = "10";

    char q_session[4096];
    if (shell_quote(session, q_session, sizeof(q_session)) != 0) {
        fprintf(stderr, "[loogal:window_api_error] session id too long\n");
        return 1;
    }

    char cmd[WINDOW_API_CMD_MAX];
    int n = snprintf(cmd, sizeof(cmd), "./loogal session page %s --offset %s --limit %s", q_session, offset, limit);
    if (n < 0 || n >= (int)sizeof(cmd)) {
        fprintf(stderr, "[loogal:window_api_error] command too long\n");
        return 1;
    }

    return run_passthrough_json("page", cmd, as_json, dry_run);
}

static int cmd_window_api_current(int argc, char **argv) {
    int as_json = has_flag_local(argc, argv, "--json");
    int dry_run = has_flag_local(argc, argv, "--dry-run");
    const char *cmd = "./loogal history current --json";
    return run_passthrough_json("current", cmd, as_json, dry_run);
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
