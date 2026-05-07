#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "loogal/platform.h"
#include "jsonout.h"
#include "session.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef SESSION_CMD_MAX
#define SESSION_CMD_MAX 8192
#endif

#define LOOGAL_SESSION_DIR "data/sessions"
#define LOOGAL_SESSION_INDEX "data/sessions/sessions.jsonl"

typedef struct {
    const char *query;
    const char *place;
    const char *min_percent;
    const char *limit;
    const char *offset;
    int as_json;
    int dry_run;
    int memory;
} SessionArgs;

static void session_usage(void) {
    puts("LOOGAL SESSION — reusable search snapshots for loogal-window");
    puts("");
    puts("Commands:");
    puts("  loogal session create --query <image> --place <dir> [--min N] [--limit N] [--json] [--dry-run]");
    puts("  loogal session list [--json]");
    puts("  loogal session show <session_id> [--meta]");
    puts("  loogal session page <session_id> --offset N --limit N [--json]");
    puts("");
    puts("Purpose:");
    puts("  A session stores the query, scope, results JSON, and replay command");
    puts("  so loogal-window can scroll, restore, and navigate without scraping text.");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return NULL;
}

static int mkdir_p_simple(const char *path) {
    if (!path || !*path) return -1;
    char tmp[LOOGAL_PATH_MAX * 2];
    if (snprintf(tmp, sizeof(tmp), "%s", path) >= (int)sizeof(tmp)) return -1;
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (loogal_platform_mkdir(tmp) != 0) return -1;
            *p = '/';
        }
    }
    if (loogal_platform_mkdir(tmp) != 0) return -1;
    return 0;
}

static int shell_quote(const char *in, char *out, size_t out_sz) {
    if (!in || !out || out_sz < 3) return -1;
    size_t j = 0;
    out[j++] = '\'';
    for (size_t i = 0; in[i]; i++) {
        if (in[i] == '\'') {
            const char *esc = "'\\''";
            for (size_t k = 0; esc[k]; k++) {
                if (j + 1 >= out_sz) return -1;
                out[j++] = esc[k];
            }
        } else {
            if (j + 1 >= out_sz) return -1;
            out[j++] = in[i];
        }
    }
    if (j + 1 >= out_sz) return -1;
    out[j++] = '\'';
    out[j] = 0;
    return 0;
}

static int copy_stream_to_file(FILE *in, const char *path, size_t *bytes_out) {
    FILE *out = fopen(path, "wb");
    if (!out) return -1;
    char buf[8192];
    size_t total = 0;
    while (!feof(in)) {
        size_t n = fread(buf, 1, sizeof(buf), in);
        if (n > 0) {
            if (fwrite(buf, 1, n, out) != n) {
                fclose(out);
                return -1;
            }
            total += n;
        }
        if (ferror(in)) {
            fclose(out);
            return -1;
        }
    }
    if (fclose(out) != 0) return -1;
    if (bytes_out) *bytes_out = total;
    return 0;
}

static int write_meta_file(const char *meta_path,
                           const char *id,
                           const SessionArgs *a,
                           const char *results_path,
                           const char *command) {
    FILE *f = fopen(meta_path, "w");
    if (!f) return -1;
    char ts[64];
    iso_time_now(ts, sizeof(ts));
    fprintf(f, "{\n");
    fprintf(f, "  "); loogal_json_kv_string(f, "tool", "loogal", 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "type", "session.meta", 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "id", id, 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "created_at", ts, 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "query", a->query ? a->query : "", 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "place", a->place ? a->place : "", 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "min_percent", a->min_percent ? a->min_percent : "60", 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "limit", a->limit ? a->limit : "50", 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "results", results_path, 1);
    fprintf(f, "  "); loogal_json_kv_string(f, "replay_command", command, 0);
    fprintf(f, "}\n");
    return fclose(f);
}

static void append_index_line(const char *id, const SessionArgs *a, const char *meta_path, const char *results_path) {
    FILE *f = fopen(LOOGAL_SESSION_INDEX, "a");
    if (!f) return;
    char ts[64];
    iso_time_now(ts, sizeof(ts));
    fprintf(f, "{\"ts\":");
    loogal_json_string(f, ts);
    fprintf(f, ",\"tool\":\"loogal\",\"type\":\"session.created\",\"id\":");
    loogal_json_string(f, id);
    fprintf(f, ",\"query\":");
    loogal_json_string(f, a->query ? a->query : "");
    fprintf(f, ",\"place\":");
    loogal_json_string(f, a->place ? a->place : "");
    fprintf(f, ",\"meta\":");
    loogal_json_string(f, meta_path);
    fprintf(f, ",\"results\":");
    loogal_json_string(f, results_path);
    fprintf(f, "}\n");
    fclose(f);
}

static int parse_create_args(int argc, char **argv, SessionArgs *a) {
    memset(a, 0, sizeof(*a));
    a->query = arg_value(argc, argv, "--query");
    a->place = arg_value(argc, argv, "--place");
    a->min_percent = arg_value(argc, argv, "--min");
    a->limit = arg_value(argc, argv, "--limit");
    a->offset = arg_value(argc, argv, "--offset");
    a->as_json = loogal_has_flag(argc, argv, "--json");
    a->dry_run = loogal_has_flag(argc, argv, "--dry-run");
    a->memory = loogal_has_flag(argc, argv, "--memory");
    if (!a->limit) a->limit = "50";
    if (!a->min_percent) a->min_percent = "60";
    if (!a->offset) a->offset = "0";
    if (!a->query && argc >= 2 && argv[1][0] != '-') a->query = argv[1];
    if (!a->place && argc >= 3 && argv[2][0] != '-') a->place = argv[2];
    if (!a->query) return -1;
    if (!a->memory && !a->place) return -1;
    return 0;
}

static int build_search_command(const SessionArgs *a, char *cmd, size_t cmd_sz) {
    char q_query[LOOGAL_PATH_MAX * 2];
    char q_place[LOOGAL_PATH_MAX * 2];
    if (shell_quote(a->query, q_query, sizeof(q_query)) != 0) return -1;
    if (a->place && shell_quote(a->place, q_place, sizeof(q_place)) != 0) return -1;

    if (a->memory) {
        return snprintf(cmd, cmd_sz,
                        "./loogal search %s --memory --json --min %s --limit %s --offset %s",
                        q_query, a->min_percent, a->limit, a->offset) >= (int)cmd_sz ? -1 : 0;
    }

    return snprintf(cmd, cmd_sz,
                    "./loogal search %s %s --json --min %s --limit %s --offset %s",
                    q_query, q_place, a->min_percent, a->limit, a->offset) >= (int)cmd_sz ? -1 : 0;
}


static int run_search_direct_to_file(const SessionArgs *a, const char *results_path, size_t *bytes_out) {
    if (!a || !results_path) return -1;

    FILE *out = fopen(results_path, "wb");
    if (!out) return -1;

    fflush(stdout);

    int saved_stdout = dup(STDOUT_FILENO);
    if (saved_stdout < 0) {
        fclose(out);
        return -1;
    }

    int out_fd = fileno(out);
    if (out_fd < 0) {
        close(saved_stdout);
        fclose(out);
        return -1;
    }

    if (dup2(out_fd, STDOUT_FILENO) < 0) {
        close(saved_stdout);
        fclose(out);
        return -1;
    }

    char *search_argv[16];
    int search_argc = 0;

    search_argv[search_argc++] = (char *)a->query;

    if (a->memory) {
        search_argv[search_argc++] = "--memory";
    } else if (a->place) {
        search_argv[search_argc++] = (char *)a->place;
    }

    search_argv[search_argc++] = "--json";
    search_argv[search_argc++] = "--min";
    search_argv[search_argc++] = (char *)(a->min_percent ? a->min_percent : "60");
    search_argv[search_argc++] = "--limit";
    search_argv[search_argc++] = (char *)(a->limit ? a->limit : "50");
    search_argv[search_argc++] = "--offset";
    search_argv[search_argc++] = (char *)(a->offset ? a->offset : "0");

    int rc = cmd_search(search_argc, search_argv);

    fflush(stdout);

    long pos = ftell(out);
    if (pos < 0) pos = 0;

    if (dup2(saved_stdout, STDOUT_FILENO) < 0) {
        close(saved_stdout);
        fclose(out);
        return -1;
    }

    close(saved_stdout);

    if (fclose(out) != 0) return -1;

    if (bytes_out) *bytes_out = (size_t)pos;

    return rc == 0 ? 0 : -1;
}

int loogal_session_create_direct(
    const char *query,
    const char *place,
    int memory,
    const char *min_percent,
    const char *limit,
    const char *offset,
    LoogalSessionCreateResult *out
) {
    if (!query || !out) return -1;
    if (!memory && !place) return -1;

    memset(out, 0, sizeof(*out));

    SessionArgs a;
    memset(&a, 0, sizeof(a));
    a.query = query;
    a.place = place;
    a.memory = memory;
    a.min_percent = min_percent ? min_percent : "60";
    a.limit = limit ? limit : "50";
    a.offset = offset ? offset : "0";

    if (build_search_command(&a, out->replay_command, sizeof(out->replay_command)) != 0) {
        LOOGAL_ERROR(LOOGAL_ERR_INTERNAL_TRUNCATED, "session", "build_replay_command", query, "failed to build replay command");
        return -1;
    }

    snprintf(out->id, sizeof(out->id), "session_%ld_%ld", (long)now_unix(), (long)getpid());

    char dir[LOOGAL_PATH_MAX * 2];
    int n_dir = snprintf(dir, sizeof(dir), "%s/%s", LOOGAL_SESSION_DIR, out->id);
    int n_results = snprintf(out->results_path, sizeof(out->results_path), "%s/results.json", dir);
    int n_meta = snprintf(out->meta_path, sizeof(out->meta_path), "%s/meta.json", dir);

    if (n_dir < 0 || n_dir >= (int)sizeof(dir) ||
        n_results < 0 || n_results >= (int)sizeof(out->results_path) ||
        n_meta < 0 || n_meta >= (int)sizeof(out->meta_path)) {
        LOOGAL_ERROR(LOOGAL_ERR_PLATFORM_PATH_TOO_LONG, "session", "create_paths", query, "session path exceeded fixed buffer");
        return -1;
    }

    if (mkdir_p_simple(dir) != 0) {
        LOOGAL_ERROR(LOOGAL_ERR_IO_MKDIR, "session", "mkdir_session_dir", dir, "failed to create session directory");
        return -1;
    }

    if (run_search_direct_to_file(&a, out->results_path, &out->bytes) != 0) {
        LOOGAL_ERROR(LOOGAL_ERR_CMD_OPERATION_FAILED, "session", "run_search_direct", query, "direct search failed while creating session");
        return -1;
    }

    if (write_meta_file(out->meta_path, out->id, &a, out->results_path, out->replay_command) != 0) {
        LOOGAL_ERROR(LOOGAL_ERR_IO_WRITE_OUTPUT, "session", "write_meta", out->meta_path, "failed to write session meta");
        return -1;
    }

    append_index_line(out->id, &a, out->meta_path, out->results_path);

    return 0;
}

static int cmd_session_create(int argc, char **argv) {
    SessionArgs a;
    if (parse_create_args(argc, argv, &a) != 0) {
        session_usage();
        loogal_log("session", "error", "missing --query or --place for session create");
        return 1;
    }

    char command[SESSION_CMD_MAX];
    if (build_search_command(&a, command, sizeof(command)) != 0) {
        loogal_log("session", "error", "failed to build replay command");
        return 1;
    }

    char id[128];
    snprintf(id, sizeof(id), "session_%ld_%ld", (long)now_unix(), (long)getpid());

    char dir[LOOGAL_PATH_MAX * 2];
    char results_path[LOOGAL_PATH_MAX * 2];
    char meta_path[LOOGAL_PATH_MAX * 2];
    snprintf(dir, sizeof(dir), "%s/%s", LOOGAL_SESSION_DIR, id);
    int n_results = snprintf(results_path, sizeof(results_path), "%s/results.json", dir);
    if (n_results < 0 || n_results >= (int)sizeof(results_path)) {
        fprintf(stderr, "[loogal:session_error] results path too long\n");
        return 1;
    }

    int n_meta = snprintf(meta_path, sizeof(meta_path), "%s/meta.json", dir);
    if (n_meta < 0 || n_meta >= (int)sizeof(meta_path)) {
        fprintf(stderr, "[loogal:session_error] meta path too long\n");
        return 1;
    }

    if (a.dry_run) {
        if (a.as_json) {
            puts("{");
            printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
            printf("  "); loogal_json_kv_string(stdout, "type", "session.dry_run", 1);
            printf("  "); loogal_json_kv_string(stdout, "id", id, 1);
            printf("  "); loogal_json_kv_string(stdout, "command", command, 1);
            printf("  "); loogal_json_kv_string(stdout, "session_dir", dir, 0);
            puts("}");
        } else {
            printf("[loogal:session_dry_run] %s\n", command);
        }
        loogal_log("session", "ok", "session create dry-run");
        return 0;
    }

    if (mkdir_p_simple(dir) != 0) {
        perror("mkdir session dir");
        loogal_log("session", "error", "failed to create session directory");
        return 1;
    }

    FILE *pipe = popen(command, "r");
    if (!pipe) {
        loogal_log("session", "error", "failed to run search command");
        return 1;
    }
    size_t bytes = 0;
    int copy_rc = copy_stream_to_file(pipe, results_path, &bytes);
    int close_rc = pclose(pipe);
    if (copy_rc != 0 || close_rc != 0) {
        loogal_log("session", "error", "search command failed while creating session");
        return 1;
    }

    if (write_meta_file(meta_path, id, &a, results_path, command) != 0) {
        loogal_log("session", "error", "failed to write session meta");
        return 1;
    }
    append_index_line(id, &a, meta_path, results_path);

    char logmsg[LOOGAL_PATH_MAX * 2];
    int n_log = snprintf(logmsg, sizeof(logmsg), "created session id=%s results=%s bytes=%zu", id, results_path, bytes);
    if (n_log < 0 || n_log >= (int)sizeof(logmsg)) {
        snprintf(logmsg, sizeof(logmsg), "created session id=%s results_path_too_long bytes=%zu", id, bytes);
    }
    loogal_log("session", "ok", logmsg);

    if (a.as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "session.created", 1);
        printf("  "); loogal_json_kv_string(stdout, "id", id, 1);
        printf("  "); loogal_json_kv_string(stdout, "meta", meta_path, 1);
        printf("  "); loogal_json_kv_string(stdout, "results", results_path, 1);
        printf("  "); loogal_json_kv_int(stdout, "bytes", (long long)bytes, 1);
        printf("  "); loogal_json_kv_string(stdout, "replay_command", command, 0);
        puts("}");
    } else {
        printf("[loogal:session_ok] created %s\n", id);
        printf("meta:    %s\n", meta_path);
        printf("results: %s\n", results_path);
    }
    return 0;
}

static int print_file_to_stdout(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) fwrite(buf, 1, n, stdout);
    int err = ferror(f);
    fclose(f);
    return err ? -1 : 0;
}

static int cmd_session_show(int argc, char **argv) {
    if (argc < 1) {
        session_usage();
        return 1;
    }
    const char *id = argv[0];
    int meta = loogal_has_flag(argc, argv, "--meta");
    char path[LOOGAL_PATH_MAX * 2];
    snprintf(path, sizeof(path), "%s/%s/%s", LOOGAL_SESSION_DIR, id, meta ? "meta.json" : "results.json");
    if (print_file_to_stdout(path) != 0) {
        fprintf(stderr, "[loogal:session_error] cannot read %s\n", path);
        loogal_log("session", "error", "session show failed");
        return 1;
    }
    return 0;
}

typedef struct {
int as_json;
int first;
} LoogalSessionListCtx;

static const char *session_entry_id_from_path(const char *path) {
if (!path) return "";

const char *slash = strrchr(path, '/');

return slash ? slash + 1 : path;
}

static int session_list_cb(const LoogalPlatformWalkEntry *entry, void *user) {
if (!entry || !user) return -1;

LoogalSessionListCtx *ctx = (LoogalSessionListCtx *)user;

if (entry->depth != 1) return 0;
if (entry->type != LOOGAL_PLATFORM_ENTRY_DIR) return 0;

const char *id = session_entry_id_from_path(entry->path);

if (strncmp(id, "session_", 8) != 0) return 0;

if (ctx->as_json) {
if (!ctx->first) puts(",");
printf("    {");
printf("\"id\":");
loogal_json_string(stdout, id);
printf(",\"meta\":");

char meta[LOOGAL_PATH_MAX * 2];
int meta_n = snprintf(meta, sizeof(meta), "%s/%s/meta.json", LOOGAL_SESSION_DIR, id);
if (meta_n < 0 || meta_n >= (int)sizeof(meta)) return -1;
loogal_json_string(stdout, meta);

printf(",\"results\":");

char res[LOOGAL_PATH_MAX * 2];
int res_n = snprintf(res, sizeof(res), "%s/%s/results.json", LOOGAL_SESSION_DIR, id);
if (res_n < 0 || res_n >= (int)sizeof(res)) return -1;
loogal_json_string(stdout, res);

printf("}");
ctx->first = 0;
} else {
printf("  %s\n", id);
}

return 0;
}

static int cmd_session_list(int argc, char **argv) {
int as_json = loogal_has_flag(argc, argv, "--json");

if (!loogal_platform_dir_exists(LOOGAL_SESSION_DIR)) {
if (as_json) puts("{\n  \"tool\": \"loogal\",\n  \"type\": \"session.list\",\n  \"sessions\": []\n}");
else puts("No sessions yet.");
return 0;
}

if (as_json) {
puts("{");
printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
printf("  "); loogal_json_kv_string(stdout, "type", "session.list", 1);
puts("  \"sessions\": [");
} else {
puts("LOOGAL SESSIONS");
}

LoogalSessionListCtx ctx;
ctx.as_json = as_json;
ctx.first = 1;

if (loogal_platform_walk(LOOGAL_SESSION_DIR, session_list_cb, &ctx) != 0) {
if (as_json) {
puts("");
puts("  ]");
puts("}");
}
return 1;
}

if (as_json) {
puts("");
puts("  ]");
puts("}");
}

return 0;
}

static int read_meta_value(const char *meta_path, const char *key, char *out, size_t out_sz) {
    FILE *f = fopen(meta_path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    if (len <= 0 || len > 65536) { fclose(f); return -1; }
    char *buf = malloc((size_t)len + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, (size_t)len, f);
    buf[len] = 0;
    fclose(f);

    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    char *p = strstr(buf, pattern);
    if (!p) { free(buf); return -1; }
    p = strchr(p, ':');
    if (!p) { free(buf); return -1; }
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '\"') { free(buf); return -1; }
    p++;
    size_t j = 0;
    while (*p && *p != '\"' && j + 1 < out_sz) {
        if (*p == '\\' && p[1]) p++;
        out[j++] = *p++;
    }
    out[j] = 0;
    free(buf);
    return j > 0 ? 0 : -1;
}

static int cmd_session_page(int argc, char **argv) {
    if (argc < 1) {
        session_usage();
        return 1;
    }
    const char *id = argv[0];
    const char *offset = arg_value(argc, argv, "--offset");
    const char *limit = arg_value(argc, argv, "--limit");
    if (!offset) offset = "0";
    if (!limit) limit = "50";

    char meta_path[LOOGAL_PATH_MAX * 2];
    snprintf(meta_path, sizeof(meta_path), "%s/%s/meta.json", LOOGAL_SESSION_DIR, id);

    char query[LOOGAL_PATH_MAX];
    char place[LOOGAL_PATH_MAX];
    char minp[64];
    if (read_meta_value(meta_path, "query", query, sizeof(query)) != 0 ||
        read_meta_value(meta_path, "place", place, sizeof(place)) != 0 ||
        read_meta_value(meta_path, "min_percent", minp, sizeof(minp)) != 0) {
        fprintf(stderr, "[loogal:session_error] cannot read session meta: %s\n", meta_path);
        loogal_log("session", "error", "session page failed to read meta");
        return 1;
    }

    SessionArgs a;
    memset(&a, 0, sizeof(a));
    a.query = query;
    a.place = place;
    a.min_percent = minp;
    a.limit = limit;
    a.offset = offset;

    char command[SESSION_CMD_MAX];
    if (build_search_command(&a, command, sizeof(command)) != 0) return 1;
    FILE *pipe = popen(command, "r");
    if (!pipe) return 1;
    char buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) fwrite(buf, 1, n, stdout);
    int rc = pclose(pipe);
    loogal_log("session", rc == 0 ? "ok" : "error", "paged session by replaying stored query");
    return rc == 0 ? 0 : 1;
}

int cmd_session(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        session_usage();
        loogal_log("session", "ok", "printed session help");
        return 0;
    }
    const char *sub = argv[0];
    if (strcmp(sub, "create") == 0) return cmd_session_create(argc - 1, argv + 1);
    if (strcmp(sub, "list") == 0) return cmd_session_list(argc - 1, argv + 1);
    if (strcmp(sub, "show") == 0) return cmd_session_show(argc - 1, argv + 1);
    if (strcmp(sub, "page") == 0) return cmd_session_page(argc - 1, argv + 1);

    session_usage();
    loogal_log("session", "error", "unknown session subcommand");
    return 1;
}
