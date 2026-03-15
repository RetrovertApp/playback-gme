///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Retrovert Settings API - Plugin settings registration and retrieval
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "rv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RVS_FLOAT_TYPE 0x1000
#define RVS_INTEGER_TYPE 0x1001
#define RVS_BOOL_TYPE 0x1002
#define RVS_INTEGER_RANGE_TYPE 0x1003
#define RVS_STRING_RANGE_TYPE 0x1004
#define RV_SETTINGS_API_VERSION 1

typedef enum RVSettingsResult {
    RVSettingsResult_Ok = 0,
    RVSettingsResult_NotFound = 1,
    RVSettingsResult_UnknownId = 2,
    RVSettingsResult_DuplicatedId = 3,
    RVSettingsResult_WrongType = 4,
} RVSettingsResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum RVSettingsUpdate {
    RVSettingsUpdate_Default = 0,
    RVSettingsUpdate_RequireRestart = 1,
} RVSettingsUpdate;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSBase {
    const char* widget_id;
    const char* name;
    const char* desc;
    uint64_t widget_type;
} RVSBase;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSFloat {
    const char* widget_id;
    const char* name;
    const char* desc;
    uint64_t widget_type;
    float value;
    float start_range;
    float end_range;
} RVSFloat;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSInteger {
    const char* widget_id;
    const char* name;
    const char* desc;
    uint64_t widget_type;
    int value;
    int start_range;
    int end_range;
} RVSInteger;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSBool {
    const char* widget_id;
    const char* name;
    const char* desc;
    uint64_t widget_type;
    bool value;
} RVSBool;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSIntegerRangeValue {
    const char* name;
    int value;
} RVSIntegerRangeValue;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSStringRangeValue {
    const char* name;
    const char* value;
} RVSStringRangeValue;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSIntegerFixedRange {
    const char* widget_id;
    const char* name;
    const char* desc;
    uint64_t widget_type;
    int value;
    RVSIntegerRangeValue* values;
    uint64_t values_size;
} RVSIntegerFixedRange;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSStringFixedRange {
    const char* widget_id;
    const char* name;
    const char* desc;
    uint64_t widget_type;
    const char* value;
    RVSStringRangeValue* values;
    uint64_t values_size;
} RVSStringFixedRange;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSIntResult {
    RVSettingsResult result;
    int value;
} RVSIntResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSFloatResult {
    RVSettingsResult result;
    float value;
} RVSFloatResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSStringResult {
    RVSettingsResult result;
    const char* value;
} RVSStringResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSBoolResult {
    RVSettingsResult result;
    bool value;
} RVSBoolResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef union RVSetting {
    RVSInteger int_value;
    RVSFloat float_value;
    RVSIntegerFixedRange int_fixed_value;
    RVSStringFixedRange string_fixed_value;
    RVSBool bool_value;
} RVSetting;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RVSettingsPrivate;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVSettings {
    struct RVSettingsPrivate* private_data;
    // Register the settings to be used for the playback plugin. The id has to be unique otherwise SettingsResult will
    // report DuplicateId
    RVSettingsResult (*reg)(struct RVSettingsPrivate* self, const char* id, RVSetting* settings,
                            uint64_t settings_size);
    // access settings. The reg_id has to match with the id when calling [Settings::reg]. ext is for apply the sitting
    // on a per file extension basis. Use "" for global
    RVSStringResult (*get_string)(struct RVSettingsPrivate* self, const char* reg_id, const char* ext, const char* id);
    RVSIntResult (*get_int)(struct RVSettingsPrivate* self, const char* reg_id, const char* ext, const char* id);
    RVSFloatResult (*get_float)(struct RVSettingsPrivate* self, const char* reg_id, const char* ext, const char* id);
    RVSBoolResult (*get_bool)(struct RVSettingsPrivate* self, const char* reg_id, const char* ext, const char* id);
} RVSettings;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RVSettings_reg(self, id, settings, settings_size) self->reg(self->private_data, id, settings, settings_size)
#define RVSettings_get_string(self, reg_id, ext, id) self->get_string(self->private_data, reg_id, ext, id)
#define RVSettings_get_int(self, reg_id, ext, id) self->get_int(self->private_data, reg_id, ext, id)
#define RVSettings_get_float(self, reg_id, ext, id) self->get_float(self->private_data, reg_id, ext, id)
#define RVSettings_get_bool(self, reg_id, ext, id) self->get_bool(self->private_data, reg_id, ext, id)

#define RVSettings_register_array(api, name, settings) \
    api->reg(api->private_data, name, (RVSetting*)&settings, rv_sizeof_array(settings))

/// These macros are used to make it easier to construct the data
#define RVSIntValue(id, name, desc, value) { .int_value = { id, name, desc, RVS_INTEGER_TYPE, value, 0, 0 } }
#define RVSFloatValue(id, name, desc, value) { .float_value = { id, name, desc, RVS_FLOAT_TYPE, value, 0.0f, 0.0f } }
#define RVSFloatValue_Range(id, name, desc, value, start, end)               \
    {                                                                        \
        .float_value = { id, name, desc, RVS_FLOAT_TYPE, value, start, end } \
    }
#define RVSBoolValue(id, name, desc, value)                    \
    {                                                          \
        .bool_value = { id, name, desc, RVS_BOOL_TYPE, value } \
    }
#define RVSIntValue_Range(id, name, desc, value, min, max)                 \
    {                                                                      \
        .int_value = { id, name, desc, RVS_INTEGER_TYPE, value, min, max } \
    }
#define RVSIntValue_DescRange(id, name, desc, value, ranges) \
    {                                                        \
        .int_fixed_value                                     \
            = { id,                                          \
                name,                                        \
                desc,                                        \
                RVS_INTEGER_RANGE_TYPE,                      \
                value,                                       \
                (RVSIntegerRangeValue*)&ranges,              \
                rv_sizeof_array(ranges) }                    \
    }
#define RVSIntValue_DescRangeLen(id, name, desc, value, ranges, len)                                              \
    {                                                                                                             \
        .int_fixed_value = { id, name, desc, RVS_INTEGER_RANGE_TYPE, value, (RVSIntegerRangeValue*)&ranges, len } \
    }
#define RVSStringValue_DescRange(id, name, desc, value, ranges) \
    {                                                           \
        .string_fixed_value                                     \
            = { id,                                             \
                name,                                           \
                desc,                                           \
                RVS_STRING_RANGE_TYPE,                          \
                value,                                          \
                (RVSStringRangeValue*)&ranges,                  \
                rv_sizeof_array(ranges) }                       \
    }

#ifdef __cplusplus
}
#endif
