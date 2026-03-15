///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Retrovert Metadata API - Song metadata reporting for plugins
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "rv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RV_METADATA_API_VERSION 1
#define RV_METADATA_TITLE_TAG "title"
#define RV_METADATA_SONGTYPE_TAG "song_type"
#define RV_METADATA_AUTHORINGTOOL_TAG "authoring_tool"
#define RV_METADATA_ARTIST_TAG "artist"
#define RV_METADATA_ALBUM_TAG "album"
#define RV_METADATA_DATE_TAG "date"
#define RV_METADATA_GENRE_TAG "genre"
#define RV_METADATA_MESSAGE_TAG "message"
#define RV_METADATA_LENGTH_TAG "length"
typedef uint64_t RVMetadataId;

typedef enum RVMetaEncoding {
    RVMetaEncoding_Utf8 = 0,
    RVMetaEncoding_ShiftJS2 = 1,
} RVMetaEncoding;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum RVRVMetadataResult {
    RVRVMetadataResult_KeyNotFound = 0,
    RVRVMetadataResult_UnableToMakeQuery = 1,
} RVRVMetadataResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RVMetadataPrivate;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVMetadata {
    struct RVMetadataPrivate* private_data;
    RVMetadataId (*create_url)(struct RVMetadataPrivate* self, const char* url);
    void (*set_tag)(struct RVMetadataPrivate* self, RVMetadataId id, const char* tag, const char* data);
    void (*set_tag_f64)(struct RVMetadataPrivate* self, RVMetadataId id, const char* tag, double data);
    void (*add_subsong)(struct RVMetadataPrivate* self, RVMetadataId parent_id, uint32_t index, const char* name,
                        float length);
    void (*add_sample)(struct RVMetadataPrivate* self, RVMetadataId parent_id, const char* text);
    void (*add_instrument)(struct RVMetadataPrivate* self, RVMetadataId parent_id, const char* text);
} RVMetadata;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RVMetadata_create_url(self, url) self->create_url(self->private_data, url)
#define RVMetadata_set_tag(self, id, tag, data) self->set_tag(self->private_data, id, tag, data)
#define RVMetadata_set_tag_f64(self, id, tag, data) self->set_tag_f64(self->private_data, id, tag, data)
#define RVMetadata_add_subsong(self, parent_id, index, name, length) \
    self->add_subsong(self->private_data, parent_id, index, name, length)
#define RVMetadata_add_sample(self, parent_id, text) self->add_sample(self->private_data, parent_id, text)
#define RVMetadata_add_instrument(self, parent_id, text) self->add_instrument(self->private_data, parent_id, text)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define RV_PLUGIN_USE_METADATA_API() static const RVMetadata* g_rv_metadata = NULL

#define rv_metadata_create_url(url) g_rv_metadata->create_url(g_rv_metadata->private_data, url)
#define rv_metadata_set_tag(id, tag, data) g_rv_metadata->set_tag(g_rv_metadata->private_data, id, tag, data)
#define rv_metadata_set_tag_f64(id, tag, data) g_rv_metadata->set_tag_f64(g_rv_metadata->private_data, id, tag, data)
#define rv_metadata_add_subsong(parent_id, index, name, length) \
    g_rv_metadata->add_subsong(g_rv_metadata->private_data, parent_id, index, name, length)
#define rv_metadata_add_sample(parent_id, text) g_rv_metadata->add_sample(g_rv_metadata->private_data, parent_id, text)
#define rv_metadata_add_instrument(parent_id, text) \
    g_rv_metadata->add_instrument(g_rv_metadata->private_data, parent_id, text)

#ifdef __cplusplus
}
#endif
