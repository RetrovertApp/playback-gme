///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Retrovert IO API - File I/O abstraction for plugins
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "rv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RV_IO_API_VERSION 1

typedef struct RVIoReadUrlResult {
    uint8_t* data;
    uint64_t data_size;
} RVIoReadUrlResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RVIoPrivate;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVIo {
    struct RVIoPrivate* private_data;
    // Check if a url exists. What url can be depends on the installed reader backends.
    // The default one supports local file, but there could also be inside zip files, remote (ftp/http/etc) systems here
    bool (*exists)(struct RVIoPrivate* self, const char* url);
    // Load the whole file/url to memory. What url can be depends on installed backends.
    // The default one supports local files, but there could also be inside zip files, remote (ftp/http/etc) systems
    // here
    RVIoReadUrlResult (*read_url_to_memory)(struct RVIoPrivate* self, const char* url);
    // Free the data read by [Io::read_url_to_memory] Notice that only data that was returned by this function is
    // supported here
    void (*free_url_to_memory)(struct RVIoPrivate* self, void* memory);
} RVIo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RVIo_exists(self, url) self->exists(self->private_data, url)
#define RVIo_read_url_to_memory(self, url) self->read_url_to_memory(self->private_data, url)
#define RVIo_free_url_to_memory(self, memory) self->free_url_to_memory(self->private_data, memory)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RV_PLUGIN_USE_IO_API() static const RVIo* g_rv_io = NULL

#define rv_io_exists(url) g_rv_io->exists(g_rv_io->private_data, url)
#define rv_io_read_url_to_memory(url) g_rv_io->read_url_to_memory(g_rv_io->private_data, url)
#define rv_io_free_url_to_memory(memory) g_rv_io->free_url_to_memory(g_rv_io->private_data, memory)

#ifdef __cplusplus
}
#endif
