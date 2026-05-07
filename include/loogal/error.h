#ifndef LOOGAL_ERROR_H
#define LOOGAL_ERROR_H

/*
 * Loogal error doctrine:
 *
 * Every distinct failure deserves a distinct code.
 * Codes should point toward repair, not merely announce failure.
 *
 * Ranges:
 *   0       success
 *   1000s   filesystem / IO
 *   2000s   parsing / schema
 *   3000s   image decode / image facts
 *   4000s   index / binary format
 *   5000s   memory / durable state
 *   6000s   platform / OS adapter
 *   7000s   command / user input
 *   8000s   adapter / GUI / desktop tools
 *   9000s   internal / invariant / allocation
 */

typedef enum LoogalError {
    LOOGAL_OK = 0,

    LOOGAL_ERR_IO_OPEN_INPUT          = 1001,
    LOOGAL_ERR_IO_READ_INPUT          = 1002,
    LOOGAL_ERR_IO_OPEN_OUTPUT         = 1003,
    LOOGAL_ERR_IO_WRITE_OUTPUT        = 1004,
    LOOGAL_ERR_IO_WRITE_TEMP          = 1005,
    LOOGAL_ERR_IO_RENAME_ATOMIC       = 1006,
    LOOGAL_ERR_IO_REMOVE              = 1007,
    LOOGAL_ERR_IO_STAT                = 1008,
    LOOGAL_ERR_IO_MKDIR               = 1009,

    LOOGAL_ERR_PARSE_FIELD_MISSING    = 2001,
    LOOGAL_ERR_PARSE_BAD_U64          = 2002,
    LOOGAL_ERR_PARSE_BAD_I64          = 2003,
    LOOGAL_ERR_PARSE_BAD_FLOAT        = 2004,
    LOOGAL_ERR_PARSE_LINE_TOO_LONG    = 2005,
    LOOGAL_ERR_PARSE_BAD_ESCAPE       = 2006,
    LOOGAL_ERR_PARSE_SCHEMA_MISMATCH  = 2007,

    LOOGAL_ERR_IMAGE_DECODE_FAILED    = 3001,
    LOOGAL_ERR_IMAGE_TOO_LARGE        = 3002,
    LOOGAL_ERR_IMAGE_UNSUPPORTED_EXT  = 3003,
    LOOGAL_ERR_IMAGE_BAD_DIMENSIONS   = 3004,
    LOOGAL_ERR_IMAGE_HASH_FAILED      = 3005,
    LOOGAL_ERR_IMAGE_PROBE_FAILED     = 3006,

    LOOGAL_ERR_INDEX_MAGIC_BAD        = 4001,
    LOOGAL_ERR_INDEX_VERSION_BAD      = 4002,
    LOOGAL_ERR_INDEX_CHECKSUM_BAD     = 4003,
    LOOGAL_ERR_INDEX_RECORD_BAD       = 4004,
    LOOGAL_ERR_INDEX_TRUNCATED        = 4005,
    LOOGAL_ERR_INDEX_REBUILD_FAILED   = 4006,

    LOOGAL_ERR_MEMORY_LOAD_FAILED     = 5001,
    LOOGAL_ERR_MEMORY_SAVE_FAILED     = 5002,
    LOOGAL_ERR_MEMORY_LOCATION_STALE  = 5003,
    LOOGAL_ERR_MEMORY_IDENTITY_MISSING= 5004,
    LOOGAL_ERR_MEMORY_SCHEMA_OLD      = 5005,

    LOOGAL_ERR_PLATFORM_WALK_FAILED   = 6001,
    LOOGAL_ERR_PLATFORM_PATH_TOO_LONG = 6002,
    LOOGAL_ERR_PLATFORM_UNSUPPORTED   = 6003,
    LOOGAL_ERR_PLATFORM_TIME_FAILED   = 6004,

    LOOGAL_ERR_CMD_USAGE              = 7001,
    LOOGAL_ERR_CMD_BAD_ARGUMENT       = 7002,
    LOOGAL_ERR_CMD_MISSING_ARGUMENT   = 7003,
    LOOGAL_ERR_CMD_OPERATION_FAILED   = 7004,

    LOOGAL_ERR_ADAPTER_TOOL_MISSING   = 8001,
    LOOGAL_ERR_ADAPTER_TOOL_FAILED    = 8002,
    LOOGAL_ERR_ADAPTER_GUI_FAILED     = 8003,

    LOOGAL_ERR_INTERNAL_OOM           = 9001,
    LOOGAL_ERR_INTERNAL_TRUNCATED     = 9002,
    LOOGAL_ERR_INTERNAL_INVARIANT     = 9003,
    LOOGAL_ERR_INTERNAL_OVERFLOW      = 9004
} LoogalError;

const char *loogal_error_name(LoogalError code);
const char *loogal_error_default_hint(LoogalError code);

#endif
