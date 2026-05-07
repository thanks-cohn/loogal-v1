#define _XOPEN_SOURCE 700

#include "loogal/watch_mutation.h"
#include "loogal/watch_daemon.h"
#include "loogal.h"
#include "memory.h"

#include <stdio.h>
#include <string.h>

static int load_memory_or_die(LoogalMemory *m) {
    if (loogal_memory_load(m) != 0) {
        loogal_log("watch_mutation.memory", "error", "failed loading memory");
        return 1;
    }

    return 0;
}

int loogal_watch_handle_created(const char *path) {
    if (!path || !path[0]) {
        loogal_log("watch_mutation.created", "error", "missing path");
        return 1;
    }

    loogal_log("watch_mutation.created", "ok", path);

    /*
     * future:
     * probe image
     * ingest identity
     * patch projection/index
     */

    return 0;
}

int loogal_watch_handle_modified(const char *path) {
    if (!path || !path[0]) {
        loogal_log("watch_mutation.modified", "error", "missing path");
        return 1;
    }

    loogal_log("watch_mutation.modified", "ok", path);

    /*
     * future:
     * reprobe image
     * detect identity drift
     * append mutation event
     */

    return 0;
}

int loogal_watch_handle_deleted(const char *path) {
    if (!path || !path[0]) {
        loogal_log("watch_mutation.deleted", "error", "missing path");
        return 1;
    }

    loogal_log("watch_mutation.deleted", "ok", path);

    /*
     * future:
     * tombstone location
     * preserve identity history
     */

    return 0;
}

int loogal_watch_handle_moved(const char *old_path, const char *new_path) {
    if (!old_path || !old_path[0] || !new_path || !new_path[0]) {
        loogal_log("watch_mutation.moved", "error", "missing move paths");
        return 1;
    }

    loogal_log("watch_mutation.moved", "ok", new_path);

    /*
     * future:
     * update durable location path
     * preserve identity linkage
     */

    return 0;
}
