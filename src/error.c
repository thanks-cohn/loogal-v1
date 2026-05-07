#include "loogal/error.h"

const char *loogal_error_name(LoogalError code) {
    switch (code) {
        case LOOGAL_OK: return "LOOGAL_OK";

        case LOOGAL_ERR_IO_OPEN_INPUT: return "LOOGAL_ERR_IO_OPEN_INPUT";
        case LOOGAL_ERR_IO_READ_INPUT: return "LOOGAL_ERR_IO_READ_INPUT";
        case LOOGAL_ERR_IO_OPEN_OUTPUT: return "LOOGAL_ERR_IO_OPEN_OUTPUT";
        case LOOGAL_ERR_IO_WRITE_OUTPUT: return "LOOGAL_ERR_IO_WRITE_OUTPUT";
        case LOOGAL_ERR_IO_WRITE_TEMP: return "LOOGAL_ERR_IO_WRITE_TEMP";
        case LOOGAL_ERR_IO_RENAME_ATOMIC: return "LOOGAL_ERR_IO_RENAME_ATOMIC";
        case LOOGAL_ERR_IO_REMOVE: return "LOOGAL_ERR_IO_REMOVE";
        case LOOGAL_ERR_IO_STAT: return "LOOGAL_ERR_IO_STAT";
        case LOOGAL_ERR_IO_MKDIR: return "LOOGAL_ERR_IO_MKDIR";

        case LOOGAL_ERR_PARSE_FIELD_MISSING: return "LOOGAL_ERR_PARSE_FIELD_MISSING";
        case LOOGAL_ERR_PARSE_BAD_U64: return "LOOGAL_ERR_PARSE_BAD_U64";
        case LOOGAL_ERR_PARSE_BAD_I64: return "LOOGAL_ERR_PARSE_BAD_I64";
        case LOOGAL_ERR_PARSE_BAD_FLOAT: return "LOOGAL_ERR_PARSE_BAD_FLOAT";
        case LOOGAL_ERR_PARSE_LINE_TOO_LONG: return "LOOGAL_ERR_PARSE_LINE_TOO_LONG";
        case LOOGAL_ERR_PARSE_BAD_ESCAPE: return "LOOGAL_ERR_PARSE_BAD_ESCAPE";
        case LOOGAL_ERR_PARSE_SCHEMA_MISMATCH: return "LOOGAL_ERR_PARSE_SCHEMA_MISMATCH";

        case LOOGAL_ERR_IMAGE_DECODE_FAILED: return "LOOGAL_ERR_IMAGE_DECODE_FAILED";
        case LOOGAL_ERR_IMAGE_TOO_LARGE: return "LOOGAL_ERR_IMAGE_TOO_LARGE";
        case LOOGAL_ERR_IMAGE_UNSUPPORTED_EXT: return "LOOGAL_ERR_IMAGE_UNSUPPORTED_EXT";
        case LOOGAL_ERR_IMAGE_BAD_DIMENSIONS: return "LOOGAL_ERR_IMAGE_BAD_DIMENSIONS";
        case LOOGAL_ERR_IMAGE_HASH_FAILED: return "LOOGAL_ERR_IMAGE_HASH_FAILED";
        case LOOGAL_ERR_IMAGE_PROBE_FAILED: return "LOOGAL_ERR_IMAGE_PROBE_FAILED";

        case LOOGAL_ERR_INDEX_MAGIC_BAD: return "LOOGAL_ERR_INDEX_MAGIC_BAD";
        case LOOGAL_ERR_INDEX_VERSION_BAD: return "LOOGAL_ERR_INDEX_VERSION_BAD";
        case LOOGAL_ERR_INDEX_CHECKSUM_BAD: return "LOOGAL_ERR_INDEX_CHECKSUM_BAD";
        case LOOGAL_ERR_INDEX_RECORD_BAD: return "LOOGAL_ERR_INDEX_RECORD_BAD";
        case LOOGAL_ERR_INDEX_TRUNCATED: return "LOOGAL_ERR_INDEX_TRUNCATED";
        case LOOGAL_ERR_INDEX_REBUILD_FAILED: return "LOOGAL_ERR_INDEX_REBUILD_FAILED";

        case LOOGAL_ERR_MEMORY_LOAD_FAILED: return "LOOGAL_ERR_MEMORY_LOAD_FAILED";
        case LOOGAL_ERR_MEMORY_SAVE_FAILED: return "LOOGAL_ERR_MEMORY_SAVE_FAILED";
        case LOOGAL_ERR_MEMORY_LOCATION_STALE: return "LOOGAL_ERR_MEMORY_LOCATION_STALE";
        case LOOGAL_ERR_MEMORY_IDENTITY_MISSING: return "LOOGAL_ERR_MEMORY_IDENTITY_MISSING";
        case LOOGAL_ERR_MEMORY_SCHEMA_OLD: return "LOOGAL_ERR_MEMORY_SCHEMA_OLD";

        case LOOGAL_ERR_PLATFORM_WALK_FAILED: return "LOOGAL_ERR_PLATFORM_WALK_FAILED";
        case LOOGAL_ERR_PLATFORM_PATH_TOO_LONG: return "LOOGAL_ERR_PLATFORM_PATH_TOO_LONG";
        case LOOGAL_ERR_PLATFORM_UNSUPPORTED: return "LOOGAL_ERR_PLATFORM_UNSUPPORTED";
        case LOOGAL_ERR_PLATFORM_TIME_FAILED: return "LOOGAL_ERR_PLATFORM_TIME_FAILED";

        case LOOGAL_ERR_CMD_USAGE: return "LOOGAL_ERR_CMD_USAGE";
        case LOOGAL_ERR_CMD_BAD_ARGUMENT: return "LOOGAL_ERR_CMD_BAD_ARGUMENT";
        case LOOGAL_ERR_CMD_MISSING_ARGUMENT: return "LOOGAL_ERR_CMD_MISSING_ARGUMENT";
        case LOOGAL_ERR_CMD_OPERATION_FAILED: return "LOOGAL_ERR_CMD_OPERATION_FAILED";

        case LOOGAL_ERR_ADAPTER_TOOL_MISSING: return "LOOGAL_ERR_ADAPTER_TOOL_MISSING";
        case LOOGAL_ERR_ADAPTER_TOOL_FAILED: return "LOOGAL_ERR_ADAPTER_TOOL_FAILED";
        case LOOGAL_ERR_ADAPTER_GUI_FAILED: return "LOOGAL_ERR_ADAPTER_GUI_FAILED";

        case LOOGAL_ERR_INTERNAL_OOM: return "LOOGAL_ERR_INTERNAL_OOM";
        case LOOGAL_ERR_INTERNAL_TRUNCATED: return "LOOGAL_ERR_INTERNAL_TRUNCATED";
        case LOOGAL_ERR_INTERNAL_INVARIANT: return "LOOGAL_ERR_INTERNAL_INVARIANT";
        case LOOGAL_ERR_INTERNAL_OVERFLOW: return "LOOGAL_ERR_INTERNAL_OVERFLOW";
    }

    return "LOOGAL_ERR_UNKNOWN";
}

const char *loogal_error_default_hint(LoogalError code) {
    switch (code) {
        case LOOGAL_OK:
            return "operation completed";

        case LOOGAL_ERR_IO_OPEN_INPUT:
            return "check path, permissions, mount state, and whether the input still exists";
        case LOOGAL_ERR_IO_READ_INPUT:
            return "check disk health, permissions, and whether the file changed during read";
        case LOOGAL_ERR_IO_OPEN_OUTPUT:
            return "check output directory, permissions, and available disk space";
        case LOOGAL_ERR_IO_WRITE_OUTPUT:
        case LOOGAL_ERR_IO_WRITE_TEMP:
            return "check free space, permissions, and whether the target filesystem is writable";
        case LOOGAL_ERR_IO_RENAME_ATOMIC:
            return "check whether temp and final paths are on the same filesystem";
        case LOOGAL_ERR_IO_REMOVE:
            return "check permissions and whether another process is holding the file";
        case LOOGAL_ERR_IO_STAT:
            return "check whether the file exists and whether metadata access is permitted";
        case LOOGAL_ERR_IO_MKDIR:
            return "check parent path permissions and whether a non-directory occupies the path";

        case LOOGAL_ERR_PARSE_FIELD_MISSING:
        case LOOGAL_ERR_PARSE_BAD_U64:
        case LOOGAL_ERR_PARSE_BAD_I64:
        case LOOGAL_ERR_PARSE_BAD_FLOAT:
        case LOOGAL_ERR_PARSE_LINE_TOO_LONG:
        case LOOGAL_ERR_PARSE_BAD_ESCAPE:
        case LOOGAL_ERR_PARSE_SCHEMA_MISMATCH:
            return "inspect the source record; rebuild derived indexes if durable memory is intact";

        case LOOGAL_ERR_IMAGE_DECODE_FAILED:
            return "file may be corrupt or unsupported; verify with another viewer and skip safely";
        case LOOGAL_ERR_IMAGE_TOO_LARGE:
            return "raise image limits deliberately or exclude the path";
        case LOOGAL_ERR_IMAGE_UNSUPPORTED_EXT:
            return "unsupported extension; add native decoder support or skip";
        case LOOGAL_ERR_IMAGE_BAD_DIMENSIONS:
            return "image reported invalid dimensions; treat file as hostile or corrupt";
        case LOOGAL_ERR_IMAGE_HASH_FAILED:
        case LOOGAL_ERR_IMAGE_PROBE_FAILED:
            return "check decoder path, file permissions, and image validity";

        case LOOGAL_ERR_INDEX_MAGIC_BAD:
        case LOOGAL_ERR_INDEX_VERSION_BAD:
        case LOOGAL_ERR_INDEX_CHECKSUM_BAD:
        case LOOGAL_ERR_INDEX_RECORD_BAD:
        case LOOGAL_ERR_INDEX_TRUNCATED:
        case LOOGAL_ERR_INDEX_REBUILD_FAILED:
            return "rebuild binary index from durable memory; do not trust this projection";

        case LOOGAL_ERR_MEMORY_LOAD_FAILED:
        case LOOGAL_ERR_MEMORY_SAVE_FAILED:
        case LOOGAL_ERR_MEMORY_LOCATION_STALE:
        case LOOGAL_ERR_MEMORY_IDENTITY_MISSING:
        case LOOGAL_ERR_MEMORY_SCHEMA_OLD:
            return "inspect durable JSONL memory and run verify before rebuilding projections";

        case LOOGAL_ERR_PLATFORM_WALK_FAILED:
        case LOOGAL_ERR_PLATFORM_PATH_TOO_LONG:
        case LOOGAL_ERR_PLATFORM_UNSUPPORTED:
        case LOOGAL_ERR_PLATFORM_TIME_FAILED:
            return "check platform adapter boundary; OS-specific behavior should be isolated there";

        case LOOGAL_ERR_CMD_USAGE:
        case LOOGAL_ERR_CMD_BAD_ARGUMENT:
        case LOOGAL_ERR_CMD_MISSING_ARGUMENT:
        case LOOGAL_ERR_CMD_OPERATION_FAILED:
            return "check command arguments and run the command help";

        case LOOGAL_ERR_ADAPTER_TOOL_MISSING:
        case LOOGAL_ERR_ADAPTER_TOOL_FAILED:
        case LOOGAL_ERR_ADAPTER_GUI_FAILED:
            return "optional adapter failure; core should still work without this dependency";

        case LOOGAL_ERR_INTERNAL_OOM:
            return "out of memory; reduce batch size or inspect allocation site";
        case LOOGAL_ERR_INTERNAL_TRUNCATED:
            return "increase buffer size or reject the oversized input explicitly";
        case LOOGAL_ERR_INTERNAL_INVARIANT:
            return "internal invariant failed; capture inputs and file a minimal reproduction";
        case LOOGAL_ERR_INTERNAL_OVERFLOW:
            return "integer overflow guard fired; inspect size/count multiplication";
    }

    return "unknown code; add a distinct error entry and repair hint";
}
