#ifndef LOOGAL_LOG_H
#define LOOGAL_LOG_H

#include "loogal/error.h"

typedef enum LoogalLogLevel {
    LOOGAL_LOG_TRACE = 0,
    LOOGAL_LOG_INFO  = 1,
    LOOGAL_LOG_WARN  = 2,
    LOOGAL_LOG_ERROR = 3
} LoogalLogLevel;

const char *loogal_log_level_name(LoogalLogLevel level);

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
);

#define LOOGAL_TRACE(module, operation, path, message) \
    loogal_log_event_ex(LOOGAL_LOG_TRACE, LOOGAL_OK, module, operation, path, message, NULL, __FILE__, __LINE__, __func__)

#define LOOGAL_INFO(module, operation, path, message) \
    loogal_log_event_ex(LOOGAL_LOG_INFO, LOOGAL_OK, module, operation, path, message, NULL, __FILE__, __LINE__, __func__)

#define LOOGAL_WARN(code, module, operation, path, message) \
    loogal_log_event_ex(LOOGAL_LOG_WARN, code, module, operation, path, message, NULL, __FILE__, __LINE__, __func__)

#define LOOGAL_ERROR(code, module, operation, path, message) \
    loogal_log_event_ex(LOOGAL_LOG_ERROR, code, module, operation, path, message, NULL, __FILE__, __LINE__, __func__)

#define LOOGAL_ERROR_HINT(code, module, operation, path, message, hint) \
    loogal_log_event_ex(LOOGAL_LOG_ERROR, code, module, operation, path, message, hint, __FILE__, __LINE__, __func__)

#endif
