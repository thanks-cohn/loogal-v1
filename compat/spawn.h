#ifndef LOOGAL_COMPAT_SPAWN_H
#define LOOGAL_COMPAT_SPAWN_H

typedef int pid_t;
typedef void posix_spawn_file_actions_t;
typedef void posix_spawnattr_t;

static inline int posix_spawnp(
    pid_t *pid,
    const char *file,
    const posix_spawn_file_actions_t *actions,
    const posix_spawnattr_t *attrp,
    char *const argv[],
    char *const envp[]
) {
    (void)pid; (void)file; (void)actions; (void)attrp; (void)argv; (void)envp;
    return -1;
}

#endif
