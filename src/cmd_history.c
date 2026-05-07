#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "loogal/platform.h"
#include "jsonout.h"
#include "history.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define LOOGAL_HISTORY_DIR "data/history"
#define LOOGAL_HISTORY_STACK "data/history/stack.tsv"
#define LOOGAL_HISTORY_AUDIT "data/history/history.jsonl"
#define LOOGAL_HISTORY_STATE "data/history/state.json"
#define HISTORY_FIELD_MAX 2048
#define HISTORY_LINE_MAX 8192
#define HISTORY_MAX_ENTRIES 512

typedef struct {
    int index;
    char ts[64];
    char query[HISTORY_FIELD_MAX];
    char session[HISTORY_FIELD_MAX];
    long scroll_offset;
    long selected_rank;
} HistoryEntry;

static void history_usage(void) {
    puts("LOOGAL HISTORY — browser-style visual search navigation");
    puts("");
    puts("Commands:");
    puts("  loogal history push --query <image> --session <session_id> [--offset N] [--selected N] [--json]");
    puts("  loogal history current [--json]");
    puts("  loogal history back [--json]");
    puts("  loogal history forward [--json]");
    puts("  loogal history list [--json]");
    puts("  loogal history clear [--json]");
    puts("");
    puts("Purpose:");
    puts("  Stores Loogal visual search history so loogal-window can behave like a browser:");
    puts("  Search Similar pushes a new entry, Back restores the previous session,");
    puts("  Forward restores the next session unless a new branch clears it.");
}

static const char *arg_value(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return NULL;
}

static int has_flag_local(int argc, char **argv, const char *flag) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) return 1;
    }
    return 0;
}

static void safe_copy(char *dst, size_t dst_sz, const char *src) {
    if (!dst || dst_sz == 0) return;
    if (!src) src = "";
    snprintf(dst, dst_sz, "%s", src);
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

static int ensure_history_dirs(void) {
    if (mkdir_p_simple(LOOGAL_HISTORY_DIR) != 0) return -1;
    return 0;
}

static int read_current_index(void) {
    FILE *f = fopen(LOOGAL_HISTORY_STATE, "r");
    if (!f) return -1;
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = 0;
    const char *p = strstr(buf, "current_index");
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    return atoi(p + 1);
}

static int write_current_index(int idx) {
    if (ensure_history_dirs() != 0) return -1;
    FILE *f = fopen(LOOGAL_HISTORY_STATE, "w");
    if (!f) return -1;
    char ts[64];
    iso_time_now(ts, sizeof(ts));
    fprintf(f, "{\n");
    fprintf(f, "  \"tool\": \"loogal\",\n");
    fprintf(f, "  \"type\": \"history.state\",\n");
    fprintf(f, "  \"current_index\": %d,\n", idx);
    fprintf(f, "  \"updated_at\": ");
    loogal_json_string(f, ts);
    fprintf(f, "\n}\n");
    return fclose(f);
}

static int parse_tsv_line(const char *line, HistoryEntry *e) {
    if (!line || !e) return -1;
    char tmp[HISTORY_LINE_MAX];
    safe_copy(tmp, sizeof(tmp), line);
    char *nl = strchr(tmp, '\n');
    if (nl) *nl = 0;

    char *fields[6] = {0};
    char *save = NULL;
    char *tok = strtok_r(tmp, "\t", &save);
    int n = 0;
    while (tok && n < 6) {
        fields[n++] = tok;
        tok = strtok_r(NULL, "\t", &save);
    }
    if (n < 6) return -1;
    e->index = atoi(fields[0]);
    safe_copy(e->ts, sizeof(e->ts), fields[1]);
    safe_copy(e->query, sizeof(e->query), fields[2]);
    safe_copy(e->session, sizeof(e->session), fields[3]);
    e->scroll_offset = atol(fields[4]);
    e->selected_rank = atol(fields[5]);
    return 0;
}

static int load_entries(HistoryEntry *entries, int max_entries) {
    FILE *f = fopen(LOOGAL_HISTORY_STACK, "r");
    if (!f) return 0;
    char line[HISTORY_LINE_MAX];
    int count = 0;
    while (fgets(line, sizeof(line), f) && count < max_entries) {
        if (parse_tsv_line(line, &entries[count]) == 0) count++;
    }
    fclose(f);
    return count;
}

static int write_entries(const HistoryEntry *entries, int count) {
    if (ensure_history_dirs() != 0) return -1;
    FILE *f = fopen(LOOGAL_HISTORY_STACK, "w");
    if (!f) return -1;
    for (int i = 0; i < count; i++) {
        fprintf(f, "%d\t%s\t%s\t%s\t%ld\t%ld\n",
                i,
                entries[i].ts,
                entries[i].query,
                entries[i].session,
                entries[i].scroll_offset,
                entries[i].selected_rank);
    }
    return fclose(f);
}

static void print_entry_json(const HistoryEntry *e, int current_index, int trailing_comma) {
    printf("{\n");
    printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
    printf("  "); loogal_json_kv_string(stdout, "type", "history.entry", 1);
    printf("  \"index\": %d,\n", e->index);
    printf("  \"current_index\": %d,\n", current_index);
    printf("  "); loogal_json_kv_string(stdout, "ts", e->ts, 1);
    printf("  "); loogal_json_kv_string(stdout, "query", e->query, 1);
    printf("  "); loogal_json_kv_string(stdout, "session", e->session, 1);
    printf("  \"scroll_offset\": %ld,\n", e->scroll_offset);
    printf("  \"selected_rank\": %ld\n", e->selected_rank);
    printf("}%s\n", trailing_comma ? "," : "");
}

static void print_entry_human(const char *label, const HistoryEntry *e, int current_index, int total) {
    printf("[loogal:history_%s]\n", label);
    printf("  index:         %d / %d\n", e->index, total > 0 ? total - 1 : -1);
    printf("  current_index: %d\n", current_index);
    printf("  query:         %s\n", e->query);
    printf("  session:       %s\n", e->session);
    printf("  scroll_offset: %ld\n", e->scroll_offset);
    printf("  selected_rank: %ld\n", e->selected_rank);
}

static void append_audit(const char *event, const HistoryEntry *e, int current_index) {
    if (ensure_history_dirs() != 0) return;
    FILE *f = fopen(LOOGAL_HISTORY_AUDIT, "a");
    if (!f) return;
    char ts[64];
    iso_time_now(ts, sizeof(ts));
    fprintf(f, "{\"ts\":"); loogal_json_string(f, ts);
    fprintf(f, ",\"tool\":\"loogal\",\"type\":\"history.%s\"", event ? event : "event");
    fprintf(f, ",\"current_index\":%d", current_index);
    if (e) {
        fprintf(f, ",\"index\":%d,\"query\":", e->index); loogal_json_string(f, e->query);
        fprintf(f, ",\"session\":"); loogal_json_string(f, e->session);
        fprintf(f, ",\"scroll_offset\":%ld,\"selected_rank\":%ld", e->scroll_offset, e->selected_rank);
    }
    fprintf(f, "}\n");
    fclose(f);
}


int loogal_history_push_direct(
    const char *query,
    const char *session,
    long scroll_offset,
    long selected_rank
) {
    if (!query || !session) return -1;

    HistoryEntry entries[HISTORY_MAX_ENTRIES];
    int count = load_entries(entries, HISTORY_MAX_ENTRIES);
    int current = read_current_index();

    if (current < -1) current = -1;
    if (current >= count) current = count - 1;

    int keep_count = current + 1;
    if (keep_count < 0) keep_count = 0;
    if (keep_count > count) keep_count = count;
    count = keep_count;

    if (count >= HISTORY_MAX_ENTRIES) {
        LOOGAL_ERROR(LOOGAL_ERR_INTERNAL_OVERFLOW, "history", "push", query, "history stack is full");
        return -1;
    }

    HistoryEntry e;
    memset(&e, 0, sizeof(e));
    e.index = count;
    iso_time_now(e.ts, sizeof(e.ts));
    safe_copy(e.query, sizeof(e.query), query);
    safe_copy(e.session, sizeof(e.session), session);
    e.scroll_offset = scroll_offset;
    e.selected_rank = selected_rank;

    entries[count++] = e;

    if (write_entries(entries, count) != 0 || write_current_index(count - 1) != 0) {
        LOOGAL_ERROR(LOOGAL_ERR_IO_WRITE_OUTPUT, "history", "write_push", query, "failed to write history stack/state");
        return -1;
    }

    append_audit("push", &e, count - 1);
    LOOGAL_INFO("history", "push_direct", query, "pushed history entry through direct C service");

    return 0;
}

static int cmd_history_push(int argc, char **argv) {
    int as_json = has_flag_local(argc, argv, "--json");
    const char *query = arg_value(argc, argv, "--query");
    const char *session = arg_value(argc, argv, "--session");
    const char *offset_s = arg_value(argc, argv, "--offset");
    const char *selected_s = arg_value(argc, argv, "--selected");
    if (!query || !session) {
        history_usage();
        loogal_log("history", "error", "push missing --query or --session");
        return 1;
    }

    HistoryEntry entries[HISTORY_MAX_ENTRIES];
    int count = load_entries(entries, HISTORY_MAX_ENTRIES);
    int current = read_current_index();
    if (current < -1) current = -1;
    if (current >= count) current = count - 1;

    /* Browser rule: if you search from the middle, forward history is cleared. */
    int keep_count = current + 1;
    if (keep_count < 0) keep_count = 0;
    if (keep_count > count) keep_count = count;
    count = keep_count;

    if (count >= HISTORY_MAX_ENTRIES) {
        loogal_log("history", "error", "history stack full");
        fprintf(stderr, "[loogal:history_error] stack full\n");
        return 1;
    }

    HistoryEntry e;
    memset(&e, 0, sizeof(e));
    e.index = count;
    iso_time_now(e.ts, sizeof(e.ts));
    safe_copy(e.query, sizeof(e.query), query);
    safe_copy(e.session, sizeof(e.session), session);
    e.scroll_offset = offset_s ? atol(offset_s) : 0;
    e.selected_rank = selected_s ? atol(selected_s) : 1;
    entries[count++] = e;

    if (write_entries(entries, count) != 0 || write_current_index(count - 1) != 0) {
        loogal_log("history", "error", "failed to write history push");
        fprintf(stderr, "[loogal:history_error] failed to write history\n");
        return 1;
    }
    append_audit("push", &e, count - 1);
    loogal_log("history", "ok", "pushed history entry");

    if (as_json) {
        print_entry_json(&e, count - 1, 0);
    } else {
        print_entry_human("push_ok", &e, count - 1, count);
    }
    return 0;
}

static int output_index(const char *label, int target, int as_json) {
    HistoryEntry entries[HISTORY_MAX_ENTRIES];
    int count = load_entries(entries, HISTORY_MAX_ENTRIES);
    if (count <= 0) {
        if (as_json) {
            puts("{");
            printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
            printf("  "); loogal_json_kv_string(stdout, "type", "history.empty", 1);
            printf("  \"current_index\": -1,\n");
            printf("  \"total\": 0\n");
            puts("}");
        } else {
            puts("[loogal:history_empty] no history entries");
        }
        return 1;
    }
    if (target < 0) target = 0;
    if (target >= count) target = count - 1;

    if (write_current_index(target) != 0) {
        loogal_log("history", "error", "failed to update current index");
        return 1;
    }
    append_audit(label, &entries[target], target);
    loogal_log("history", "ok", label);

    if (as_json) print_entry_json(&entries[target], target, 0);
    else print_entry_human(label, &entries[target], target, count);
    return 0;
}

static int cmd_history_current(int argc, char **argv) {
    int as_json = has_flag_local(argc, argv, "--json");
    HistoryEntry entries[HISTORY_MAX_ENTRIES];
    int count = load_entries(entries, HISTORY_MAX_ENTRIES);
    int current = read_current_index();
    if (count <= 0 || current < 0 || current >= count) return output_index("current_empty", -1, as_json);
    if (as_json) print_entry_json(&entries[current], current, 0);
    else print_entry_human("current", &entries[current], current, count);
    return 0;
}

static int cmd_history_back(int argc, char **argv) {
    int as_json = has_flag_local(argc, argv, "--json");
    HistoryEntry entries[HISTORY_MAX_ENTRIES];
    int count = load_entries(entries, HISTORY_MAX_ENTRIES);
    int current = read_current_index();
    if (count <= 0 || current <= 0) return output_index("back_boundary", current < 0 ? -1 : current, as_json);
    return output_index("back", current - 1, as_json);
}

static int cmd_history_forward(int argc, char **argv) {
    int as_json = has_flag_local(argc, argv, "--json");
    HistoryEntry entries[HISTORY_MAX_ENTRIES];
    int count = load_entries(entries, HISTORY_MAX_ENTRIES);
    int current = read_current_index();
    if (count <= 0) return output_index("forward_boundary", -1, as_json);
    if (current < 0) current = 0;
    if (current >= count - 1) return output_index("forward_boundary", current, as_json);
    return output_index("forward", current + 1, as_json);
}

static int cmd_history_list(int argc, char **argv) {
    int as_json = has_flag_local(argc, argv, "--json");
    HistoryEntry entries[HISTORY_MAX_ENTRIES];
    int count = load_entries(entries, HISTORY_MAX_ENTRIES);
    int current = read_current_index();
    if (current >= count) current = count - 1;

    if (as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "history.list", 1);
        printf("  \"current_index\": %d,\n", current);
        printf("  \"total\": %d,\n", count);
        puts("  \"entries\": [");
        for (int i = 0; i < count; i++) {
            printf("    {\"index\":%d,\"current\":%s,\"ts\":", i, i == current ? "true" : "false");
            loogal_json_string(stdout, entries[i].ts);
            printf(",\"query\":"); loogal_json_string(stdout, entries[i].query);
            printf(",\"session\":"); loogal_json_string(stdout, entries[i].session);
            printf(",\"scroll_offset\":%ld,\"selected_rank\":%ld}%s\n",
                   entries[i].scroll_offset,
                   entries[i].selected_rank,
                   i + 1 < count ? "," : "");
        }
        puts("  ]");
        puts("}");
    } else {
        puts("LOOGAL HISTORY");
        printf("current_index: %d\n", current);
        printf("total:         %d\n", count);
        for (int i = 0; i < count; i++) {
            printf("%s %d. %s  session=%s offset=%ld selected=%ld\n",
                   i == current ? "*" : " ",
                   i,
                   entries[i].query,
                   entries[i].session,
                   entries[i].scroll_offset,
                   entries[i].selected_rank);
        }
    }
    loogal_log("history", "ok", "listed history");
    return 0;
}

static int cmd_history_clear(int argc, char **argv) {
    int as_json = has_flag_local(argc, argv, "--json");
    if (ensure_history_dirs() != 0) return 1;
    FILE *f = fopen(LOOGAL_HISTORY_STACK, "w");
    if (f) fclose(f);
    write_current_index(-1);
    append_audit("clear", NULL, -1);
    loogal_log("history", "ok", "cleared history");
    if (as_json) {
        puts("{");
        printf("  "); loogal_json_kv_string(stdout, "tool", "loogal", 1);
        printf("  "); loogal_json_kv_string(stdout, "type", "history.cleared", 1);
        printf("  \"current_index\": -1\n");
        puts("}");
    } else {
        puts("[loogal:history_ok] cleared history");
    }
    return 0;
}

int cmd_history(int argc, char **argv) {
    if (argc < 1 || strcmp(argv[0], "help") == 0 || strcmp(argv[0], "--help") == 0) {
        history_usage();
        return 0;
    }
    const char *sub = argv[0];
    if (strcmp(sub, "push") == 0) return cmd_history_push(argc - 1, argv + 1);
    if (strcmp(sub, "current") == 0) return cmd_history_current(argc - 1, argv + 1);
    if (strcmp(sub, "back") == 0) return cmd_history_back(argc - 1, argv + 1);
    if (strcmp(sub, "forward") == 0) return cmd_history_forward(argc - 1, argv + 1);
    if (strcmp(sub, "list") == 0) return cmd_history_list(argc - 1, argv + 1);
    if (strcmp(sub, "clear") == 0) return cmd_history_clear(argc - 1, argv + 1);

    history_usage();
    loogal_log("history", "error", "unknown history command");
    return 1;
}
