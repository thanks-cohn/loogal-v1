#define _XOPEN_SOURCE 700

#include "loogal/watch_daemon.h"
#include "loogal/watch_mutation.h"
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
        return loogal_watch_handle_created(event->path);

    case LOOGAL_WATCH_EVENT_MODIFIED:
        return loogal_watch_handle_modified(event->path);

    case LOOGAL_WATCH_EVENT_DELETED:
        return loogal_watch_handle_deleted(event->path);

    case LOOGAL_WATCH_EVENT_MOVED:
        return loogal_watch_handle_moved(event->old_path, event->path);

    default:
        loogal_log("watch_daemon.event", "error", "unknown event type");
        return 1;
    }
}
