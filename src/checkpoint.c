#include "loogal.h"
#include "checkpoint.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void usage(void) {
    puts("CHECKPOINT SESSIONS");
    puts("==============================");
    puts("loogal checkpoint create <name>");
    puts("loogal checkpoint save <session> <working.jsonl>");
    puts("loogal checkpoint commit <session>");
}

static int ensure_dir(const char *path) {
#ifdef _WIN32
    return mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

static int checkpoint_create(const char *name) {
    char dir[1024];
    snprintf(dir, sizeof(dir), "data/sessions/%s", name);

    ensure_dir("data");
    ensure_dir("data/sessions");

    if (ensure_dir(dir) != 0) {
        loogal_die("checkpoint", "could not create session directory");
        return 1;
    }

    char manifest[1024];
    snprintf(manifest, sizeof(manifest), "%s/manifest.json", dir);

    FILE *f = fopen(manifest, "w");
    if (!f) {
        loogal_die("checkpoint", "could not create manifest");
        return 1;
    }

    fprintf(f,
        "{\n"
        "  \"session\": \"%s\",\n"
        "  \"created_unix\": %ld,\n"
        "  \"status\": \"working\"\n"
        "}\n",
        name,
        now_unix());

    fclose(f);

    printf("CHECKPOINT SESSION CREATED\n");
    printf("session: %s\n", name);
    printf("path:    %s\n", dir);

    return 0;
}

static int checkpoint_save(const char *session, const char *working) {
    char out[1024];
    snprintf(out, sizeof(out), "data/sessions/%s/checkpoints.jsonl", session);

    FILE *f = fopen(out, "a");
    if (!f) {
        loogal_die("checkpoint", "could not append checkpoint");
        return 1;
    }

    fprintf(f,
        "{\"type\":\"checkpoint.saved\","
        "\"session\":\"%s\","
        "\"working\":\"%s\","
        "\"created_unix\":%ld}"
        "\n",
        session,
        working,
        now_unix());

    fclose(f);

    printf("CHECKPOINT SAVED\n");
    printf("session: %s\n", session);
    printf("working: %s\n", working);

    return 0;
}

static int checkpoint_commit(const char *session) {
    FILE *f = fopen(LOOGAL_EVENTS_PATH, "a");
    if (!f) {
        loogal_die("checkpoint", "could not append commit event");
        return 1;
    }

    fprintf(f,
        "{\"type\":\"checkpoint.committed\","
        "\"session\":\"%s\","
        "\"created_unix\":%ld}"
        "\n",
        session,
        now_unix());

    fclose(f);

    printf("CHECKPOINT COMMITTED\n");
    printf("session: %s\n", session);
    printf("events:  %s\n", LOOGAL_EVENTS_PATH);

    return 0;
}

int cmd_checkpoint(int argc, char **argv) {
    if (argc < 1) {
        usage();
        return 1;
    }

    if (strcmp(argv[0], "create") == 0) {
        if (argc < 2) {
            loogal_die("checkpoint", "missing session name");
            return 1;
        }
        return checkpoint_create(argv[1]);
    }

    if (strcmp(argv[0], "save") == 0) {
        if (argc < 3) {
            loogal_die("checkpoint", "usage: checkpoint save <session> <working.jsonl>");
            return 1;
        }
        return checkpoint_save(argv[1], argv[2]);
    }

    if (strcmp(argv[0], "commit") == 0) {
        if (argc < 2) {
            loogal_die("checkpoint", "missing session name");
            return 1;
        }
        return checkpoint_commit(argv[1]);
    }

    usage();
    return 1;
}
