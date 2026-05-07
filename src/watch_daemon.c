#define _XOPEN_SOURCE 700

#include "loogal/watch_daemon.h"
#include "loogal.h"

#include <stdio.h>

int loogal_watch_daemon_usage(void) {
    puts("LOOGAL WATCH-DAEMON — live filesystem event ingestion");
    puts("");
    puts("Purpose:");
    puts("  Platform adapters feed filesystem events into this layer.");
    puts("  This layer does not watch the OS directly.");
    puts("  It translates event intent into Loogal memory/index mutations.");
    puts("");
    puts("Events:");
    puts("  created");
    puts("  modified");
    puts("  deleted");
    puts("  moved");
    return 0;
}

int loogal_watch_daemon_handle_event(const LoogalWatchEvent *event) {
    if (!event || !event->path[0]) {
        loogal_log("watch_daemon.event", "error", "missing event or path");
        return 1;
    }

    switch (event->type) {
    case LOOGAL_WATCH_EVENT_CREATED:
        loogal_log("watch_daemon.created", "ok", event->path);
        return 0;
    case LOOGAL_WATCH_EVENT_MODIFIED:
        loogal_log("watch_daemon.modified", "ok", event->path);
        return 0;
    case LOOGAL_WATCH_EVENT_DELETED:
        loogal_log("watch_daemon.deleted", "ok", event->path);
        return 0;
    case LOOGAL_WATCH_EVENT_MOVED:
        loogal_log("watch_daemon.moved", "ok", event->path);
        return 0;
    default:
        loogal_log("watch_daemon.event", "error", "unknown event type");
        return 1;
    }
}
