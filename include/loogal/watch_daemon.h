#ifndef LOOGAL_WATCH_DAEMON_H
#define LOOGAL_WATCH_DAEMON_H

typedef enum {
    LOOGAL_WATCH_EVENT_CREATED = 1,
    LOOGAL_WATCH_EVENT_MODIFIED = 2,
    LOOGAL_WATCH_EVENT_DELETED = 3,
    LOOGAL_WATCH_EVENT_MOVED = 4
} LoogalWatchEventType;

typedef struct {
    LoogalWatchEventType type;
    char path[4096];
    char old_path[4096];
} LoogalWatchEvent;

int loogal_watch_daemon_usage(void);
int loogal_watch_daemon_handle_event(const LoogalWatchEvent *event);

#endif
