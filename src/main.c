#include "loogal.h"
#include "timer.h"
#include "jsonout.h"
#include "config.h"
#include "why.h"
#include "verify.h"
#include "rebuild.h"
#include "learn.h"
#include "bench.h"
#include "thumbnail.h"
#include "action.h"
#include "session.h"
#include "history.h"
#include "similar.h"
#include "window_api.h"
#include "loogal/watch_config.h"
#include "loogal/watch_run.h"
#include <stdio.h>
#include <string.h>

int loogal_doctor(void);

static void usage(void) {
    puts("LOOGAL V1 — Local Visual Memory Engine");
    puts("");
    puts("Commands:");
    puts("  loogal index <directories...>");
    puts("  loogal learn <directories...>");
    puts("  loogal search <image> [MIN_PERCENT]");
    puts("  loogal stats");
    puts("  loogal dedupe --keep N [--dry-run] [--move-removed DIR] [--protect DIR...]");
    puts("  loogal thumbnail <path|create|session|status>");
    puts("  loogal action reveal --path <file>");
    puts("  loogal action open --path <file>");
    puts("  loogal action copy-path --path <file>");
    puts("  loogal session create --query <image> --place <dir> [--json]");
    puts("  loogal history push --query <image> --session <id>");
    puts("  loogal similar --path <image> --place <dir> [--json]");
    puts("  loogal window-api page --session <id> [--json]");
    puts("  loogal doctor");
    puts("  loogal config");
    puts("  loogal why <path>");
    puts("  loogal verify");
    puts("  loogal bench");
    puts("  loogal rebuild <directories...>");
    puts("  loogal where");
    puts("  loogal status");
puts("  loogal watch-list");
puts("  loogal watch-add <path> --daily HH:MM | --hourly | --weekly day HH:MM | --yearly MM-DD HH:MM");
puts("  loogal watch-run [--dry-run|--all]");
}

static int finish_command(const char *cmd, int rc, double start_ms) {
    double elapsed = loogal_now_ms() - start_ms;

    char msg[256];
    snprintf(msg, sizeof(msg), "command=%s rc=%d duration_ms=%.3f",
             cmd ? cmd : "unknown", rc, elapsed);

    loogal_log("command", rc == 0 ? "ok" : "error", msg);
    return rc;
}

int main(int argc, char **argv) {
    ensure_dirs();

    double start_ms = loogal_now_ms();

    if (argc < 2) {
        usage();
        return finish_command("usage", 1, start_ms);
    }

    const char *cmd = argv[1];

    if (!strcmp(cmd, "watch-add")) return loogal_cmd_watch_add(argc, argv);
    if (!strcmp(cmd, "watch-list")) return loogal_cmd_watch_list(argc, argv);
    if (!strcmp(cmd, "watch-run")) return loogal_cmd_watch_run(argc, argv);
    if (!strcmp(cmd, "watch-remove")) return loogal_cmd_watch_remove(argc, argv);
    if (!strcmp(cmd, "watch-enable")) return loogal_cmd_watch_enable(argc, argv);
    if (!strcmp(cmd, "watch-disable")) return loogal_cmd_watch_disable(argc, argv);

    int rc = 1;

    if (strcmp(cmd, "index") == 0) {
        rc = cmd_index(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

if (strcmp(cmd, "learn") == 0) {
rc = cmd_learn(argc - 2, argv + 2);
return finish_command(cmd, rc, start_ms);
}

if (strcmp(cmd, "bench") == 0) {
rc = cmd_bench(argc - 2, argv + 2);
return finish_command(cmd, rc, start_ms);
}

    if (strcmp(cmd, "search") == 0) {
        rc = cmd_search(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "thumbnail") == 0) {
        rc = cmd_thumbnail(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "stats") == 0 || strcmp(cmd, "status") == 0) {
        rc = cmd_stats(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "dedupe") == 0) {
        rc = cmd_dedupe(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "action") == 0) {
        rc = cmd_action(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "session") == 0) {
        rc = cmd_session(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "history") == 0) {
        rc = cmd_history(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "similar") == 0) {
        rc = cmd_similar(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "window-api") == 0) {
        rc = cmd_window_api(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "doctor") == 0) {
        rc = loogal_doctor();
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "config") == 0) {
        rc = cmd_config(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

    if (strcmp(cmd, "why") == 0) {
        rc = cmd_why(argc - 2, argv + 2);
        return finish_command(cmd, rc, start_ms);
    }

if (strcmp(cmd, "verify") == 0) {
rc = cmd_verify(argc - 2, argv + 2);
return finish_command(cmd, rc, start_ms);
}

if (strcmp(cmd, "rebuild") == 0) {
rc = cmd_rebuild(argc - 2, argv + 2);
return finish_command(cmd, rc, start_ms);
}

    if (strcmp(cmd, "where") == 0) {
rc = cmd_where(argc - 2, argv + 2);
return finish_command(cmd, rc, start_ms);
}

if (strcmp(cmd, "help") == 0 || strcmp(cmd, "--help") == 0) {
        usage();
        return finish_command(cmd, 0, start_ms);
    }

    usage();
    loogal_log("main", "error", "unknown command");
    return finish_command(cmd, 1, start_ms);
}
