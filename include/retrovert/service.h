///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Retrovert Service API - Central service locator for plugin APIs
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct RVServicePrivData;

struct RVIo;
struct RVLog;
struct RVMetadata;
struct RVSettings;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVService {
    struct RVServicePrivData* private_data;
    const struct RVIo* (*get_io)(struct RVServicePrivData* private_data, int api_version);
    const struct RVLog* (*get_log)(struct RVServicePrivData* private_data, int api_version);
    const struct RVMetadata* (*get_metadata)(struct RVServicePrivData* private_data, int api_version);
    const struct RVSettings* (*get_settings)(struct RVServicePrivData* private_data, int api_version);
} RVService;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RVService_get_io(api, version) api->get_io(api->private_data, version)
#define RVService_get_log(api, version) api->get_log(api->private_data, version)
#define RVService_get_metadata(api, version) api->get_metadata(api->private_data, version)
#define RVService_get_settings(api, version) api->get_settings(api->private_data, version)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define rv_init_io_api(service) g_rv_io = RVService_get_io(service, RV_IO_API_VERSION)
#define rv_init_log_api(service) g_rv_log = RVService_get_log(service, RV_LOG_API_VERSION)
#define rv_init_metadata_api(service) g_rv_metadata = RVService_get_metadata(service, RV_METADATA_API_VERSION)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
