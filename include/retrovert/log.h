///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Retrovert Log API - Logging for plugins
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "rv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RV_LOG_API_VERSION 1

// Used for selecting which level to log at. It's recommended to use the helper macros
typedef enum RVLogLevel {
    RVLogLevel_Trace = 0,
    RVLogLevel_Debug = 1,
    RVLogLevel_Info = 2,
    RVLogLevel_Warn = 3,
    RVLogLevel_Error = 4,
    RVLogLevel_Fatal = 5,
} RVLogLevel;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RVLogPrivate;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVLog {
    struct RVLogPrivate* private_data;
    // Write to the log. It's recommended to use the macros for eaiser usage
    void (*log)(struct RVLogPrivate* self, uint32_t level, const char* file, int line, const char* fmt, ...);
} RVLog;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RV_PLUGIN_USE_LOG_API() const RVLog* g_rv_log = NULL

#define rv_debug(...)                                                                  \
    {                                                                                  \
        extern const RVLog* g_rv_log;                                                  \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Debug, NULL, 0, __VA_ARGS__); \
    }
#define rvfl_debug(...)                                                                           \
    {                                                                                             \
        extern const RVLog* g_rv_log;                                                             \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Debug, __FILE__, __LINE__, __VA_ARGS__); \
    }
#define rv_trace(...)                                                                  \
    {                                                                                  \
        extern const RVLog* g_rv_log;                                                  \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Trace, NULL, 0, __VA_ARGS__); \
    }
#define rvfl_trace(...)                                                                           \
    {                                                                                             \
        extern const RVLog* g_rv_log;                                                             \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Trace, __FILE__, __LINE__, __VA_ARGS__); \
    }
#define rv_info(...)                                                                  \
    {                                                                                 \
        extern const RVLog* g_rv_log;                                                 \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Info, NULL, 0, __VA_ARGS__); \
    }
#define rvfl_info(...)                                                                           \
    {                                                                                            \
        extern const RVLog* g_rv_log;                                                            \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Info, __FILE__, __LINE__, __VA_ARGS__); \
    }
#define rv_warn(...)                                                                  \
    {                                                                                 \
        extern const RVLog* g_rv_log;                                                 \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Warn, NULL, 0, __VA_ARGS__); \
    }
#define rvfl_warn(...)                                                                           \
    {                                                                                            \
        extern const RVLog* g_rv_log;                                                            \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Warn, __FILE__, __LINE__, __VA_ARGS__); \
    }
#define rv_error(...)                                                                  \
    {                                                                                  \
        extern const RVLog* g_rv_log;                                                  \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Error, NULL, 0, __VA_ARGS__); \
    }
#define rvfl_error(...)                                                                           \
    {                                                                                             \
        extern const RVLog* g_rv_log;                                                             \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Error, __FILE__, __LINE__, __VA_ARGS__); \
    }
#define rv_fatal(...)                                                                  \
    {                                                                                  \
        extern const RVLog* g_rv_log;                                                  \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Fatal, NULL, 0, __VA_ARGS__); \
    }
#define rvfl_fatal(...)                                                                           \
    {                                                                                             \
        extern const RVLog* g_rv_log;                                                             \
        g_rv_log->log(g_rv_log->private_data, RVLogLevel_Fatal, __FILE__, __LINE__, __VA_ARGS__); \
    }

#ifdef __cplusplus
}
#endif
