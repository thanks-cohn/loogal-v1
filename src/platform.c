#define _POSIX_C_SOURCE 200809L

#include "loogal/platform.h"

#include <dirent.h>
#ifdef _WIN32
#include <direct.h>
#endif
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int loogal_platform_dir_exists(const char *path) {
    if (!path || !path[0]) return 0;

    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }

    return S_ISDIR(st.st_mode) ? 1 : 0;
}

int loogal_platform_path_exists(const char *path) {
    if (!path || !path[0]) return 0;

    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }

    return 1;
}

int loogal_platform_file_exists(const char *path) {
    if (!path || !path[0]) return 0;

    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }

    return S_ISREG(st.st_mode) ? 1 : 0;
}

int loogal_platform_executable_exists(const char *path) {
    if (!path || !path[0]) return 0;

    return access(path, X_OK) == 0 ? 1 : 0;
}

int loogal_platform_file_readable(const char *path) {
    if (!path || !path[0]) return 0;

    if (!loogal_platform_file_exists(path)) {
        return 0;
    }

    return access(path, R_OK) == 0 ? 1 : 0;
}

int loogal_platform_file_metadata(const char *path, uint64_t *out_size, uint64_t *out_mtime) {
    if (!path || !out_size || !out_mtime) return -1;

    struct stat st;

    if (stat(path, &st) != 0) {
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        return -1;
    }

    if (st.st_size < 0) {
        return -1;
    }

    *out_size = (uint64_t)st.st_size;
    *out_mtime = (uint64_t)st.st_mtime;
    return 0;
}

int loogal_platform_file_size(const char *path, uint64_t *out_size) {
    if (!path || !out_size) return -1;

    struct stat st;

    if (stat(path, &st) != 0) {
        return -1;
    }

    if (!S_ISREG(st.st_mode)) {
        return -1;
    }

    if (st.st_size < 0) {
        return -1;
    }

    *out_size = (uint64_t)st.st_size;
    return 0;
}

int loogal_platform_mkdir(const char *path) {
    if (!path || !path[0]) return -1;

#ifdef _WIN32
    if (_mkdir(path) == 0) {
#else
    #ifdef _WIN32
    if (_mkdir(path) == 0) {
#else
    if (mkdir(path, 0755) == 0) {
#endif
#endif
        return 0;
    }

    if (errno == EEXIST) {
        return 0;
    }

    return -1;
}

int loogal_platform_copy_file(const char *src, const char *dst) {
    if (!src || !dst) return -1;

    FILE *in = fopen(src, "rb");
    if (!in) return -1;

    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }

    unsigned char buf[65536];

    for (;;) {
        size_t n = fread(buf, 1, sizeof(buf), in);

        if (n > 0) {
            if (fwrite(buf, 1, n, out) != n) {
                fclose(in);
                fclose(out);
                return -1;
            }
        }

        if (n < sizeof(buf)) {
            if (ferror(in)) {
                fclose(in);
                fclose(out);
                return -1;
            }

            break;
        }
    }

    if (fclose(in) != 0) {
        fclose(out);
        return -1;
    }

    if (fclose(out) != 0) {
        return -1;
    }

    return 0;
}


static int loogal_platform_join_path(char *out, size_t out_sz, const char *a, const char *b) {
    if (!out || !a || !b || out_sz == 0) return -1;

    size_t an = strlen(a);
    const char *sep = (an > 0 && a[an - 1] == '/') ? "" : "/";

    int n = snprintf(out, out_sz, "%s%s%s", a, sep, b);

    if (n < 0 || (size_t)n >= out_sz) return -1;

    return 0;
}

static LoogalPlatformEntryType loogal_platform_entry_type_from_mode(mode_t mode) {
    if (S_ISREG(mode)) return LOOGAL_PLATFORM_ENTRY_FILE;
    if (S_ISDIR(mode)) return LOOGAL_PLATFORM_ENTRY_DIR;
#ifndef _WIN32
#ifndef _WIN32
    if (S_ISLNK(mode)) return LOOGAL_PLATFORM_ENTRY_SYMLINK;
#endif
#endif
    return LOOGAL_PLATFORM_ENTRY_OTHER;
}

static int loogal_platform_walk_inner(const char *root, uint64_t depth, LoogalPlatformWalkFn fn, void *user) {
    DIR *dir = opendir(root);

    if (!dir) {
        return -1;
    }

    for (;;) {
        errno = 0;
        struct dirent *ent = readdir(dir);

        if (!ent) {
            int saved_errno = errno;
            closedir(dir);
            return saved_errno == 0 ? 0 : -1;
        }

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char child[4096];

        if (loogal_platform_join_path(child, sizeof(child), root, ent->d_name) != 0) {
            continue;
        }

        struct stat st;

#ifdef _WIN32
        if (stat(child, &st) != 0) {
#else
        #ifdef _WIN32
        if (stat(child, &st) != 0) {
#else
        if (lstat(child, &st) != 0) {
#endif
#endif
            continue;
        }

        LoogalPlatformWalkEntry entry;
        memset(&entry, 0, sizeof(entry));

        snprintf(entry.path, sizeof(entry.path), "%s", child);
        entry.type = loogal_platform_entry_type_from_mode(st.st_mode);
        entry.depth = depth;

        if (S_ISREG(st.st_mode) && st.st_size > 0) {
            entry.size = (uint64_t)st.st_size;
        }

        int rc = fn(&entry, user);

        if (rc != 0) {
            closedir(dir);
            return rc;
        }

        if (entry.type == LOOGAL_PLATFORM_ENTRY_DIR) {
            rc = loogal_platform_walk_inner(child, depth + 1, fn, user);

            if (rc != 0) {
                closedir(dir);
                return rc;
            }
        }
    }
}

int loogal_platform_walk(const char *root, LoogalPlatformWalkFn fn, void *user) {
    if (!root || !root[0] || !fn) return -1;

    if (!loogal_platform_dir_exists(root)) {
        return -1;
    }

    return loogal_platform_walk_inner(root, 1, fn, user);
}


uint64_t loogal_platform_now_ns(void) {
#if defined(CLOCK_MONOTONIC)
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
    }
#endif

    return (uint64_t)time(NULL) * 1000000000ULL;
}
