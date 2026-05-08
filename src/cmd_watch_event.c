#define _XOPEN_SOURCE 700

#include "loogal/watch_event.h"
#include "loogal/watch_daemon.h"

#include <stdio.h>
#include <string.h>

static void usage(void) {
    puts("LOOGAL WATCH-EVENT — manually simulate daemon events");
    puts("");
    puts("Usage:");
    puts("  loogal watch-event --created <path>");
    puts("  loogal watch-event --modified <path>");
    puts("  loogal watch-event --deleted <path>");
    puts("  loogal watch-event --moved <old-path> <new-path>");
}

int loogal_cmd_watch_event(int argc, char **argv) {
    if (argc < 4 || strcmp(argv[2], "--help") == 0 || strcmp(argv[2], "-h") == 0) {
        usage();
        return 1;
    }

    LoogalWatchEvent ev;
    memset(&ev, 0, sizeof(ev));

    if (strcmp(argv[2], "--created") == 0) {
        ev.type = LOOGAL_WATCH_EVENT_CREATED;
        snprintf(ev.path, sizeof(ev.path), "%s", argv[3]);
    } else if (strcmp(argv[2], "--modified") == 0) {
        ev.type = LOOGAL_WATCH_EVENT_MODIFIED;
        snprintf(ev.path, sizeof(ev.path), "%s", argv[3]);
    } else if (strcmp(argv[2], "--deleted") == 0) {
        ev.type = LOOGAL_WATCH_EVENT_DELETED;
        snprintf(ev.path, sizeof(ev.path), "%s", argv[3]);
    } else if (strcmp(argv[2], "--moved") == 0) {
        if (argc < 5) {
            usage();
            return 1;
        }
        ev.type = LOOGAL_WATCH_EVENT_MOVED;
        snprintf(ev.old_path, sizeof(ev.old_path), "%s", argv[3]);
        snprintf(ev.path, sizeof(ev.path), "%s", argv[3]);
    } else {
        usage();
        return 1;
    }

    return loogal_watch_daemon_handle_event(&ev);
}
