#define _XOPEN_SOURCE 700

#include "loogal.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    uint64_t directories_seen;
    uint64_t files_seen;
    uint64_t supported_images;
    uint64_t unsupported_files;
    uint64_t symlinks_skipped;
    uint64_t unreadable_entries;

    uint64_t bytes_seen;

    uint64_t min_file_bytes;
    uint64_t max_file_bytes;

    uint64_t small_lt_4kb;
    uint64_t small_lt_64kb;
    uint64_t medium_lt_1mb;
    uint64_t large_ge_1mb;
    uint64_t huge_ge_100mb;

    uint64_t ext_png;
    uint64_t ext_jpg;
    uint64_t ext_jpeg;
    uint64_t ext_webp;
    uint64_t ext_gif;
    uint64_t ext_bmp;
    uint64_t ext_other;

    uint64_t max_depth;

    uint64_t stat_ns;
    uint64_t classify_ns;
} BenchScanStats;

static uint64_t bench_now_ns(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

static const char *bench_ext(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *base = slash ? slash + 1 : path;
    const char *dot = strrchr(base, '.');

    if (!dot || dot == base || !dot[1]) return "";

    return dot + 1;
}

static int bench_ext_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

static void bench_count_ext(BenchScanStats *st, const char *path) {
    const char *e = bench_ext(path);

    if (bench_ext_eq(e, "png")) st->ext_png++;
    else if (bench_ext_eq(e, "jpg")) st->ext_jpg++;
    else if (bench_ext_eq(e, "jpeg")) st->ext_jpeg++;
    else if (bench_ext_eq(e, "webp")) st->ext_webp++;
    else if (bench_ext_eq(e, "gif")) st->ext_gif++;
    else if (bench_ext_eq(e, "bmp")) st->ext_bmp++;
    else st->ext_other++;
}

static void bench_count_size(BenchScanStats *st, uint64_t size) {
    st->bytes_seen += size;

    if (st->files_seen == 1 || size < st->min_file_bytes) {
        st->min_file_bytes = size;
    }

    if (size > st->max_file_bytes) {
        st->max_file_bytes = size;
    }

    if (size < 4096ULL) {
        st->small_lt_4kb++;
    } else if (size < 65536ULL) {
        st->small_lt_64kb++;
    } else if (size < 1048576ULL) {
        st->medium_lt_1mb++;
    } else {
        st->large_ge_1mb++;
    }

    if (size >= 104857600ULL) {
        st->huge_ge_100mb++;
    }
}

static int bench_join_path(char *out, size_t out_sz, const char *a, const char *b) {
    if (!out || !a || !b || out_sz == 0) return -1;

    size_t an = strlen(a);
    const char *sep = (an > 0 && a[an - 1] == '/') ? "" : "/";

    int n = snprintf(out, out_sz, "%s%s%s", a, sep, b);

    if (n < 0 || (size_t)n >= out_sz) return -1;

    return 0;
}

static int bench_walk_dir(const char *root, BenchScanStats *st, uint64_t depth) {
    DIR *dir = opendir(root);

    if (!dir) {
        st->unreadable_entries++;
        return -1;
    }

    if (depth > st->max_depth) {
        st->max_depth = depth;
    }

    st->directories_seen++;

    for (;;) {
        errno = 0;
        struct dirent *ent = readdir(dir);

        if (!ent) {
            if (errno != 0) {
                st->unreadable_entries++;
            }

            break;
        }

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char child[LOOGAL_PATH_MAX];

        if (bench_join_path(child, sizeof(child), root, ent->d_name) != 0) {
            st->unreadable_entries++;
            continue;
        }

        struct stat sb;

        uint64_t t0 = bench_now_ns();
        int rc = lstat(child, &sb);
        uint64_t t1 = bench_now_ns();

        if (t1 >= t0) st->stat_ns += t1 - t0;

        if (rc != 0) {
            st->unreadable_entries++;
            continue;
        }

        if (S_ISLNK(sb.st_mode)) {
            st->symlinks_skipped++;
            continue;
        }

        if (S_ISDIR(sb.st_mode)) {
            bench_walk_dir(child, st, depth + 1);
            continue;
        }

        if (!S_ISREG(sb.st_mode)) {
            st->unsupported_files++;
            continue;
        }

        st->files_seen++;

        uint64_t size = 0;
        if (sb.st_size > 0) {
            size = (uint64_t)sb.st_size;
        }

        bench_count_size(st, size);

        t0 = bench_now_ns();
        int supported = image_is_supported(child);
        bench_count_ext(st, child);
        t1 = bench_now_ns();

        if (t1 >= t0) st->classify_ns += t1 - t0;

        if (supported) {
            st->supported_images++;
        } else {
            st->unsupported_files++;
        }
    }

    closedir(dir);
    return 0;
}

static void bench_print_u64(const char *label, uint64_t value) {
    printf("  %-24s %llu\n", label, (unsigned long long)value);
}

static void bench_print_rate(const char *label, double value) {
    printf("  %-24s %.6f\n", label, value);
}

static void bench_usage(void) {
    puts("LOOGAL BENCH-SCAN — privacy-safe filesystem benchmark");
    puts("");
    puts("Usage:");
    puts("  loogal bench-scan <directory>");
    puts("");
    puts("This command prints aggregate benchmark data only.");
    puts("It does not save image names, paths, hashes, identities, locations, or records.");
}

int cmd_bench_scan(int argc, char **argv) {
    if (argc < 3) {
        bench_usage();
        return 1;
    }

    const char *target = argv[2];

    struct stat root_st;
    if (stat(target, &root_st) != 0 || !S_ISDIR(root_st.st_mode)) {
        fprintf(stderr, "[loogal:bench_scan_error] not a readable directory: %s\n", target);
        return 1;
    }

    BenchScanStats st;
    memset(&st, 0, sizeof(st));

    uint64_t start_ns = bench_now_ns();
    bench_walk_dir(target, &st, 0);
    uint64_t end_ns = bench_now_ns();

    uint64_t total_ns = end_ns >= start_ns ? end_ns - start_ns : 0;
    double total_s = (double)total_ns / 1000000000.0;

    double files_per_sec = total_s > 0.0 ? (double)st.files_seen / total_s : 0.0;
    double images_per_sec = total_s > 0.0 ? (double)st.supported_images / total_s : 0.0;
    double dirs_per_sec = total_s > 0.0 ? (double)st.directories_seen / total_s : 0.0;
    double bytes_per_sec = total_s > 0.0 ? (double)st.bytes_seen / total_s : 0.0;

    double files_per_ns = total_ns > 0 ? (double)st.files_seen / (double)total_ns : 0.0;
    double images_per_ns = total_ns > 0 ? (double)st.supported_images / (double)total_ns : 0.0;
    double ns_per_file = st.files_seen > 0 ? (double)total_ns / (double)st.files_seen : 0.0;
    double ns_per_image = st.supported_images > 0 ? (double)total_ns / (double)st.supported_images : 0.0;

    puts("LOOGAL BENCH SCAN");
    puts("");
    printf("target:        %s\n", target);
    puts("mode:          metadata-only");
    puts("privacy:       no paths, no hashes, no records written");
    puts("");

    puts("summary:");
    printf("  speed:                   %.6f files/sec\n", files_per_sec);
    printf("  cost:                    %.6f ns/file\n", ns_per_file);
    printf("  scanned:                 %llu files / %llu images\n",
        (unsigned long long)st.files_seen,
        (unsigned long long)st.supported_images);
    printf("  total:                   %.6f sec / %llu ns\n",
        total_s,
        (unsigned long long)total_ns);
    puts("");

    puts("totals:");
    bench_print_u64("directories_seen:", st.directories_seen);
    bench_print_u64("files_seen:", st.files_seen);
    bench_print_u64("supported_images:", st.supported_images);
    bench_print_u64("unsupported_files:", st.unsupported_files);
    bench_print_u64("symlinks_skipped:", st.symlinks_skipped);
    bench_print_u64("unreadable_entries:", st.unreadable_entries);
    bench_print_u64("bytes_seen:", st.bytes_seen);
    bench_print_u64("max_depth:", st.max_depth);
    puts("");

    puts("file_size_distribution:");
    bench_print_u64("min_file_bytes:", st.files_seen ? st.min_file_bytes : 0);
    bench_print_u64("max_file_bytes:", st.max_file_bytes);
    bench_print_rate("avg_file_bytes:", st.files_seen ? ((double)st.bytes_seen / (double)st.files_seen) : 0.0);
    bench_print_u64("small_lt_4kb:", st.small_lt_4kb);
    bench_print_u64("small_lt_64kb:", st.small_lt_64kb);
    bench_print_u64("medium_lt_1mb:", st.medium_lt_1mb);
    bench_print_u64("large_ge_1mb:", st.large_ge_1mb);
    bench_print_u64("huge_ge_100mb:", st.huge_ge_100mb);
    puts("");

    puts("extension_counts:");
    bench_print_u64("png:", st.ext_png);
    bench_print_u64("jpg:", st.ext_jpg);
    bench_print_u64("jpeg:", st.ext_jpeg);
    bench_print_u64("webp:", st.ext_webp);
    bench_print_u64("gif:", st.ext_gif);
    bench_print_u64("bmp:", st.ext_bmp);
    bench_print_u64("other:", st.ext_other);
    puts("");

    puts("timing:");
    printf("  total:                   %.6f sec / %llu ns\n", total_s, (unsigned long long)total_ns);
    printf("  stat_time:               %.6f sec / %llu ns\n", (double)st.stat_ns / 1000000000.0, (unsigned long long)st.stat_ns);
    printf("  classify_time:           %.6f sec / %llu ns\n", (double)st.classify_ns / 1000000000.0, (unsigned long long)st.classify_ns);
    puts("");

    puts("throughput:");
    bench_print_rate("files/sec:", files_per_sec);
    bench_print_rate("images/sec:", images_per_sec);
    bench_print_rate("dirs/sec:", dirs_per_sec);
    bench_print_rate("bytes/sec:", bytes_per_sec);
    bench_print_rate("files/ns:", files_per_ns);
    bench_print_rate("images/ns:", images_per_ns);
    bench_print_rate("ns/file:", ns_per_file);
    bench_print_rate("ns/image:", ns_per_image);
    puts("");

    puts("notes:");
    puts("  This is a privacy-safe metadata benchmark.");
    puts("  It does not decode, hash, index, or remember images.");
    puts("  Future modes may add probe/shadow/full benchmarking explicitly.");

    return 0;
}
