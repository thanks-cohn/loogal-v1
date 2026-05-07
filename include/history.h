#ifndef LOOGAL_HISTORY_H
#define LOOGAL_HISTORY_H

int loogal_history_push_direct(
    const char *query,
    const char *session,
    long scroll_offset,
    long selected_rank
);

int cmd_history(int argc, char **argv);

#endif
