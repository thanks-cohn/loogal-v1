#include "loogal.h"
#include "hash_v0.h"
#include "hash_native.h"
#include "timer.h"
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
#include "ingest.h"
#include "trace.h"
#include "region_search.h"
#include "zone.h"
#include "checkpoint.h"
#include "zsig.h"
#include "zone_search.h"
#include "cobj.h"
#include "rel.h"
#include "manifest_cmd.h"
#include "loogal/watch_config.h"
#include "loogal/watch_run.h"
#include "loogal/watch_event.h"
#include <stdio.h>
#include <string.h>

int loogal_doctor(void);

typedef int (*CmdFn)(int argc, char **argv);
typedef struct { const char *name; const char *help; CmdFn fn; } Cmd;

static int wrap_doctor(int argc, char **argv) { (void)argc; (void)argv; return loogal_doctor(); }

static Cmd commands[] = {
    {"index", "index directories into binary visual memory", cmd_index},
    {"ingest", "ingest images and container sources", cmd_ingest},
    {"learn", "learn durable continuity observations", cmd_learn},
    {"search", "search whole-image manifestations", cmd_search},
    {"trace", "trace a known manifestation", cmd_trace},
    {"region-search", "search from a crop or partial visual region", cmd_region_search},
    {"zone", "create/import bounded memory zones", cmd_zone},
    {"checkpoint", "create/save/commit edit sessions", cmd_checkpoint},
    {"zsig", "create deterministic zone signatures", cmd_zsig},
    {"zone-search", "retrieve zone signature records", cmd_zone_search},
    {"cobj", "create/trace portable continuity objects", cmd_cobj},
    {"manifest", "attach manifestations to continuity objects", cmd_manifest},
    {"rel", "add/trace continuity graph relationships", cmd_rel},
    {"thumbnail", "create and inspect thumbnails", cmd_thumbnail},
    {"stats", "show memory corpus stats", cmd_stats},
    {"status", "show memory corpus status", cmd_stats},
    {"dedupe", "deduplicate related manifestations", cmd_dedupe},
    {"action", "open/reveal/copy paths", cmd_action},
    {"session", "create search sessions", cmd_session},
    {"history", "append session history", cmd_history},
    {"similar", "find related manifestations", cmd_similar},
    {"window-api", "serve GUI-friendly session pages", cmd_window_api},
    {"doctor", "diagnose environment and storage", wrap_doctor},
    {"config", "show configuration", cmd_config},
    {"why", "explain match evidence", cmd_why},
    {"verify", "verify durable state", cmd_verify},
    {"bench", "benchmark memory search", cmd_bench},
    {"rebuild", "rebuild hot indices", cmd_rebuild},
    {"where", "show storage locations", cmd_where},
    {"hash-v0", "compute legacy hash", cmd_hash_v0},
    {"hash-v0-grid", "compute legacy grid hash", cmd_hash_v0_grid},
    {"hash-compare", "compare hashes", cmd_hash_compare},
    {"hash-native", "compute native hash", cmd_hash_native},
    {"hash-native-grid", "compute native grid hash", cmd_hash_native_grid},
    {0,0,0}
};

static void usage(void) {
    puts("LOOGAL V1 — Continuity Memory Engine");
    puts("");
    puts("Commands:");
    for (int i = 0; commands[i].name; i++) {
        printf("  loogal %-14s %s\n", commands[i].name, commands[i].help);
    }
    puts("  loogal watch-add/list/run/remove/enable/disable ... scheduled memory");
}

static int finish_command(const char *cmd, int rc, double start_ms) {
    double elapsed = loogal_now_ms() - start_ms;
    char msg[256];
    snprintf(msg, sizeof(msg), "command=%s rc=%d duration_ms=%.3f", cmd ? cmd : "unknown", rc, elapsed);
    loogal_log("command", rc == 0 ? "ok" : "error", msg);
    return rc;
}

int main(int argc, char **argv) {
    ensure_dirs();
    double start_ms = loogal_now_ms();

    if (argc < 2) { usage(); return finish_command("usage", 1, start_ms); }

    const char *cmd = argv[1];

    if (!strcmp(cmd, "watch-add")) return loogal_cmd_watch_add(argc, argv);
    if (!strcmp(cmd, "watch-list")) return loogal_cmd_watch_list(argc, argv);
    if (!strcmp(cmd, "watch-run")) return loogal_cmd_watch_run(argc, argv);
    if (!strcmp(cmd, "watch-event")) return loogal_cmd_watch_event(argc, argv);
    if (!strcmp(cmd, "watch-remove")) return loogal_cmd_watch_remove(argc, argv);
    if (!strcmp(cmd, "watch-enable")) return loogal_cmd_watch_enable(argc, argv);
    if (!strcmp(cmd, "watch-disable")) return loogal_cmd_watch_disable(argc, argv);
    if (!strcmp(cmd, "bench-scan")) return cmd_bench_scan(argc, argv);
    if (!strcmp(cmd, "help") || !strcmp(cmd, "--help")) { usage(); return finish_command(cmd, 0, start_ms); }

    for (int i = 0; commands[i].name; i++) {
        if (!strcmp(cmd, commands[i].name)) {
            int rc = commands[i].fn(argc - 2, argv + 2);
            return finish_command(cmd, rc, start_ms);
        }
    }

    usage();
    loogal_log("main", "error", "unknown command");
    return finish_command(cmd, 1, start_ms);
}
