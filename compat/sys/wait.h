#ifndef LOOGAL_COMPAT_SYS_WAIT_H
#define LOOGAL_COMPAT_SYS_WAIT_H

#define WIFEXITED(status) (1)
#define WEXITSTATUS(status) (status)

static inline int waitpid(int pid, int *status, int options) {
    (void)pid; (void)options;
    if (status) *status = -1;
    return -1;
}

#endif
