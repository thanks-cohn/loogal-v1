#define _XOPEN_SOURCE 700
#include "loogal.h"
#include "loogal/log.h"

#include <stdio.h>
#include <stdlib.h>

const char *loogal_log_level_name(LoogalLogLevel level) {
    switch (level) {
        case LOOGAL_LOG_TRACE: return "trace";
        case LOOGAL_LOG_INFO: return "info";
        case LOOGAL_LOG_WARN: return "warn";
        case LOOGAL_LOG_ERROR: return "error";
    }

    return "unknown";
}

static const char *safe_s(const char *s) {
    return s ? s : "";
}

void loogal_log_event_ex(
    LoogalLogLevel level,
    LoogalError code,
    const char *module,
    const char *operation,
    const char *path,
    const char *message,
    const char *hint,
    const char *file,
    int line,
    const char *func
) {
    ensure_dirs();

    FILE *f = fopen(LOOGAL_LOG_PATH, "a");
    if (!f) return;

    char ts[64];
    iso_time_now(ts, sizeof(ts));

    const char *resolved_hint = hint ? hint : loogal_error_default_hint(code);

    char *j_module = json_escape(safe_s(module));
    char *j_operation = json_escape(safe_s(operation));
    char *j_path = json_escape(safe_s(path));
    char *j_message = json_escape(safe_s(message));
    char *j_hint = json_escape(safe_s(resolved_hint));
    char *j_file = json_escape(safe_s(file));
    char *j_func = json_escape(safe_s(func));

    fprintf(f,
        "{\"ts\":\"%s\","
        "\"tool\":\"loogal\","
        "\"schema\":\"loogal.breadcrumb.v1\","
        "\"level\":\"%s\","
        "\"code\":%d,"
        "\"error\":\"%s\","
        "\"module\":\"%s\","
        "\"operation\":\"%s\","
        "\"path\":\"%s\","
        "\"message\":\"%s\","
        "\"hint\":\"%s\","
        "\"source_file\":\"%s\","
        "\"source_line\":%d,"
        "\"function\":\"%s\"}\n",
        ts,
        loogal_log_level_name(level),
        (int)code,
        loogal_error_name(code),
        j_module ? j_module : "",
        j_operation ? j_operation : "",
        j_path ? j_path : "",
        j_message ? j_message : "",
        j_hint ? j_hint : "",
        j_file ? j_file : "",
        line,
        j_func ? j_func : ""
    );

    free(j_module);
    free(j_operation);
    free(j_path);
    free(j_message);
    free(j_hint);
    free(j_file);
    free(j_func);

    fclose(f);
}
