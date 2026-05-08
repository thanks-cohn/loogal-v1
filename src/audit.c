#define _XOPEN_SOURCE 700
#include "audit.h"
#include "loogal.h"
#include "loogal/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void audit_ts(char *buf, size_t n) {
    time_t t = time(NULL);
    struct tm tmv;
    
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif

    strftime(buf, n, "%Y-%m-%dT%H:%M:%S%z", &tmv);
}

void loogal_audit_event(const char *event, const char *status, const char *message) {
    ensure_dirs();

    FILE *f = fopen("data/logs/loogal.jsonl", "a");
    if (!f) return;

    char ts[64];
    audit_ts(ts, sizeof(ts));

    char *e = json_escape(event ? event : "unknown");
    char *s = json_escape(status ? status : "unknown");
    char *m = json_escape(message ? message : "");

    fprintf(f,
        "{\"ts\":\"%s\",\"tool\":\"loogal\",\"schema\":\"loogal.audit.v1\","
        "\"event\":\"%s\",\"status\":\"%s\",\"message\":\"%s\"}\n",
        ts,
        e ? e : "",
        s ? s : "",
        m ? m : ""
    );

    if (e) free(e);
    if (s) free(s);
    if (m) free(m);

    fclose(f);
}

void loogal_log(const char *event, const char *status, const char *message) {
    loogal_audit_event(event, status, message);

    LoogalLogLevel level = LOOGAL_LOG_INFO;
    LoogalError code = LOOGAL_OK;

    if (status && status[0]) {
        if (strcmp(status, "error") == 0) {
            level = LOOGAL_LOG_ERROR;
            code = LOOGAL_ERR_CMD_OPERATION_FAILED;
        } else if (strcmp(status, "warn") == 0 || strcmp(status, "warning") == 0) {
            level = LOOGAL_LOG_WARN;
            code = LOOGAL_ERR_CMD_OPERATION_FAILED;
        }
    }

    loogal_log_event_ex(
        level,
        code,
        event ? event : "legacy",
        status ? status : "legacy",
        NULL,
        message ? message : "",
        NULL,
        __FILE__,
        __LINE__,
        __func__
    );
}
