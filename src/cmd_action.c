#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "loogal/platform.h"
#include "jsonout.h"
#include "action.h"

#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

typedef struct {
    const char *verb;
    const char *path;
    int as_json;
    int dry_run;
} ActionArgs;

static void action_usage(void) {
    puts("LOOGAL ACTION — portable file action endpoints");
    puts("");
    puts("Commands:");
    puts("  loogal action reveal --path <file> [--json] [--dry-run]");
    puts("  loogal action open --path <file> [--json] [--dry-run]");
    puts("  loogal action copy-path --path <file> [--json] [--dry-run]");
    puts("");
    puts("Purpose:");
    puts("  These commands are stable endpoints for loogal-window, Dolphin,");
    puts("  scripts, and future modules. The core stays file-manager neutral.");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return NULL;
}

static int command_exists(const char *cmd) {
    const char *path = getenv("PATH");
    if (!cmd || !*cmd || !path) return 0;

    char *copy = strdup(path);
    if (!copy) return 0;

    int found = 0;
    for (char *save = NULL, *dir = strtok_r(copy, ":", &save);
         dir;
         dir = strtok_r(NULL, ":", &save)) {
        char candidate[LOOGAL_PATH_MAX];
        if (snprintf(candidate, sizeof(candidate), "%s/%s", dir, cmd) >= (int)sizeof(candidate)) continue;
        if (loogal_platform_executable_exists(candidate)) {
            found = 1;
            break;
        }
    }

    free(copy);
    return found;
}

static int file_exists(const char *path) {
return loogal_platform_path_exists(path);
}

static int make_dirname(const char *path, char *out, size_t out_sz) {
    if (!path || !out || out_sz == 0) return -1;
    char tmp[LOOGAL_PATH_MAX];
    if (snprintf(tmp, sizeof(tmp), "%s", path) >= (int)sizeof(tmp)) return -1;
    char *d = dirname(tmp);
    if (!d) return -1;
    if (snprintf(out, out_sz, "%s", d) >= (int)out_sz) return -1;
    return 0;
}

static void print_json_result(const char *verb,
                              const char *path,
                              const char *status,
                              const char *adapter,
                              const char *command,
                              const char *message,
                              int dry_run) {
    puts("{");
    printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
    printf("  "); loogal_json_kv_string(stdout, "type", "action.result", 1);
    printf("  "); loogal_json_kv_string(stdout, "verb", verb ? verb : "", 1);
    printf("  "); loogal_json_kv_string(stdout, "path", path ? path : "", 1);
    printf("  "); loogal_json_kv_string(stdout, "status", status ? status : "", 1);
    printf("  "); loogal_json_kv_string(stdout, "adapter", adapter ? adapter : "", 1);
    printf("  "); loogal_json_kv_string(stdout, "command", command ? command : "", 1);
    printf("  "); loogal_json_kv_int(stdout, "dry_run", dry_run ? 1 : 0, 1);
    printf("  "); loogal_json_kv_string(stdout, "message", message ? message : "", 0);
    puts("}");
}

static int spawn_detached3(const char *cmd, const char *arg1, const char *arg2) {
    pid_t pid;
    char *argv3[4];
    argv3[0] = (char *)cmd;
    argv3[1] = (char *)arg1;
    argv3[2] = (char *)arg2;
    argv3[3] = NULL;

    if (!arg2) argv3[2] = NULL;

    int rc = posix_spawnp(&pid, cmd, NULL, NULL, argv3, environ);
    if (rc != 0) return rc;
    return 0;
}

static int run_open_endpoint(const ActionArgs *a, const char **adapter, char *command, size_t command_sz) {
    *adapter = "xdg-open";
    snprintf(command, command_sz, "xdg-open %s", a->path);

    if (!command_exists("xdg-open")) return ENOENT;
    if (a->dry_run) return 0;
    return spawn_detached3("xdg-open", a->path, NULL);
}

static int run_reveal_endpoint(const ActionArgs *a, const char **adapter, char *command, size_t command_sz) {
    char dir[LOOGAL_PATH_MAX];
    if (make_dirname(a->path, dir, sizeof(dir)) != 0) return EINVAL;

    if (command_exists("dolphin")) {
        *adapter = "dolphin-select";
        snprintf(command, command_sz, "dolphin --select %s", a->path);
        if (a->dry_run) return 0;
        return spawn_detached3("dolphin", "--select", a->path);
    }

    if (command_exists("xdg-open")) {
        *adapter = "xdg-open-directory";
        snprintf(command, command_sz, "xdg-open %s", dir);
        if (a->dry_run) return 0;
        return spawn_detached3("xdg-open", dir, NULL);
    }

    *adapter = "none";
    (void)command_sz;
    command[0] = 0;
    return ENOENT;
}

static int copy_with_program(const char *program, const char *path) {
    FILE *p = popen(program, "w");
    if (!p) return -1;
    fputs(path, p);
    int rc = pclose(p);
    if (rc == -1) return -1;
    if (WIFEXITED(rc) && WEXITSTATUS(rc) == 0) return 0;
    return -1;
}

static int run_copy_path_endpoint(const ActionArgs *a, const char **adapter, char *command, size_t command_sz) {
    if (command_exists("wl-copy")) {
        *adapter = "wl-copy";
        snprintf(command, command_sz, "wl-copy");
        if (a->dry_run) return 0;
        return copy_with_program("wl-copy", a->path) == 0 ? 0 : EIO;
    }

    if (command_exists("xclip")) {
        *adapter = "xclip";
        snprintf(command, command_sz, "xclip -selection clipboard");
        if (a->dry_run) return 0;
        return copy_with_program("xclip -selection clipboard", a->path) == 0 ? 0 : EIO;
    }

    if (command_exists("xsel")) {
        *adapter = "xsel";
        snprintf(command, command_sz, "xsel --clipboard --input");
        if (a->dry_run) return 0;
        return copy_with_program("xsel --clipboard --input", a->path) == 0 ? 0 : EIO;
    }

    *adapter = "stdout-fallback";
    snprintf(command, command_sz, "printf path to stdout");
    if (!a->as_json) puts(a->path);
    return 0;
}

static int parse_args(int argc, char **argv, ActionArgs *out) {
    memset(out, 0, sizeof(*out));
    if (argc < 1) return -1;

    out->verb = argv[0];
    out->path = arg_value(argc, argv, "--path");
    out->as_json = loogal_has_flag(argc, argv, "--json");
    out->dry_run = loogal_has_flag(argc, argv, "--dry-run");

    if (!out->path && argc >= 2 && argv[1][0] != '-') out->path = argv[1];
    return 0;
}

int cmd_action(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        action_usage();
        loogal_log("action", "ok", "printed action help");
        return 0;
    }

    ActionArgs a;
    if (parse_args(argc, argv, &a) != 0 || !a.verb || !a.path) {
        action_usage();
        loogal_log("action", "error", "missing verb or --path");
        return 1;
    }

    char resolved[LOOGAL_PATH_MAX];
    const char *path_for_action = a.path;
    if (realpath(a.path, resolved)) path_for_action = resolved;
    a.path = path_for_action;

    if (strcmp(a.verb, "copy-path") != 0 && !file_exists(a.path)) {
        char msg[LOOGAL_PATH_MAX + 128];
        snprintf(msg, sizeof(msg), "path does not exist: %s", a.path);
        if (a.as_json) print_json_result(a.verb, a.path, "error", "none", "", msg, a.dry_run);
        else fprintf(stderr, "[loogal:action_error] %s\n", msg);
        loogal_log("action", "error", msg);
        return 1;
    }

    const char *adapter = "none";
    char command[LOOGAL_PATH_MAX + 128];
    command[0] = 0;
    int rc = EINVAL;

    if (strcmp(a.verb, "reveal") == 0) {
        rc = run_reveal_endpoint(&a, &adapter, command, sizeof(command));
    } else if (strcmp(a.verb, "open") == 0) {
        rc = run_open_endpoint(&a, &adapter, command, sizeof(command));
    } else if (strcmp(a.verb, "copy-path") == 0) {
        rc = run_copy_path_endpoint(&a, &adapter, command, sizeof(command));
    } else {
        char msg[128];
        snprintf(msg, sizeof(msg), "unknown action verb: %s", a.verb);
        if (a.as_json) print_json_result(a.verb, a.path, "error", "none", "", msg, a.dry_run);
        else fprintf(stderr, "[loogal:action_error] %s\n", msg);
        loogal_log("action", "error", msg);
        return 1;
    }

    if (rc == 0) {
        char msg[LOOGAL_PATH_MAX + 256];
        snprintf(msg, sizeof(msg), "action=%s adapter=%s path=%s dry_run=%d", a.verb, adapter, a.path, a.dry_run);
        loogal_log("action", "ok", msg);

        if (a.as_json) print_json_result(a.verb, a.path, "ok", adapter, command, "action completed", a.dry_run);
        else if (strcmp(a.verb, "copy-path") != 0 || strcmp(adapter, "stdout-fallback") != 0) {
            printf("[loogal:action_ok] %s via %s — %s\n", a.verb, adapter, a.path);
        }
        return 0;
    }

    char msg[LOOGAL_PATH_MAX + 256];
    snprintf(msg, sizeof(msg), "action=%s failed adapter=%s path=%s rc=%d", a.verb, adapter, a.path, rc);
    loogal_log("action", "error", msg);

    if (a.as_json) print_json_result(a.verb, a.path, "error", adapter, command, strerror(rc), a.dry_run);
    else fprintf(stderr, "[loogal:action_error] %s: %s\n", a.verb, strerror(rc));

    return 1;
}
