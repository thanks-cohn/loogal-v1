#ifndef LOOGAL_SESSION_H
#define LOOGAL_SESSION_H

#include "loogal.h"

typedef struct LoogalSessionCreateResult {
    char id[128];
    char meta_path[LOOGAL_PATH_MAX * 2];
    char results_path[LOOGAL_PATH_MAX * 2];
    char replay_command[8192];
    size_t bytes;
} LoogalSessionCreateResult;

int loogal_session_create_direct(
    const char *query,
    const char *place,
    int memory,
    const char *min_percent,
    const char *limit,
    const char *offset,
    LoogalSessionCreateResult *out
);

int cmd_session(int argc, char **argv);

#endif
