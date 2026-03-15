///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Retrovert Playback Plugin API - Interface for audio playback plugins
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "rv_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "audio_format.h"
#include "service.h"
#include "settings.h"

#define RV_PLAYBACK_PLUGIN_API_VERSION 2

typedef enum RVProbeResult {
    RVProbeResult_Supported = 0,
    RVProbeResult_Unsupported = 1,
    RVProbeResult_Unsure = 2,
} RVProbeResult;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sets status of the data returned in the ReadInfo
typedef enum RVReadStatus {
    // This is set by default when the host requests data. Decoders are expected to set any of the below statuses
    RVReadStatus_DecodingRequest = 0,
    // Decoding of frames where ok
    RVReadStatus_Ok = 1,
    // Frames where decoded and that there are no more data left (such at the end of a song)
    RVReadStatus_Finished = 2,
    // Something went wrong when decoding the frames. The playback/decoder should use the logging system to
    // report more details on the actual error
    RVReadStatus_Error = 3,
} RVReadStatus;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This struct contains info on what format the host expects as output.
// If the decoder can match the output the host has to do less converting, but isn't required.
// The `frame_count` parameter tells you how many frames can be written to the output buffer and read from the input
// buffer. A "frame" is one sample for each channel. For example, in a stereo stream (2 channels), one frame is 2
// samples: one for the left, one for the right. The channel count is defined by the device config. The size in bytes of
// an individual sample is defined by the sample format which is also specified in the device config. Multi-channel
// audio data is always interleaved, which means the samples for each frame are stored next to each other in memory. For
// example, in a stereo stream the first pair of samples will be the left and right samples for the first frame, the
// second pair of samples will be the left and right samples for the second frame, etc.
typedef struct RVReadInfo {
    RVAudioFormat format;
    uint32_t frame_count;
    RVReadStatus status;
} RVReadInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVReadData {
    // Output for channel data. If there is more than one channel it's expected the data to be interleaved.
    void* channels_output;
    // Max number of bytes to write to the channels_output. Notice it's fine to write less as long as the correct amount
    uint32_t channels_output_max_bytes_size;
    RVReadInfo info;
} RVReadData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pattern data structures for tracker visualization

// Single cell in a tracker pattern (note + instrument + effects)
typedef struct RVPatternCell {
    uint8_t note;         // Note value: 0 = none, 1-120 = C-0 to B-9, 255 = note off
    uint8_t instrument;   // Instrument/sample number (0 = none)
    uint8_t volume;       // Volume column value (0 = none/default, 1-64 = volume, 65+ = volume commands)
    uint8_t effect;       // Effect command (format-specific, e.g., 0-15 for MOD)
    uint8_t effect_param; // Effect parameter
    uint8_t dest_channel; // Destination voice/channel for color coding (TFMX: 0-7)
} RVPatternCell;

// Maximum channels supported for visualization
#define RV_MAX_CHANNELS 8

// Per-channel information
typedef struct RVChannelInfo {
    uint32_t num_rows;    // Number of rows in this channel's track
    uint32_t current_row; // Current row position (for non-synchronized channels)
    char name[16];        // Channel/track name
} RVChannelInfo;

// Information about a tracker module's structure
typedef struct RVTrackerInfo {
    uint16_t num_patterns;                   // Total number of patterns
    uint8_t num_channels;                    // Number of channels
    uint8_t channels_synchronized;           // 1 if all channels share the same row position
    uint16_t num_orders;                     // Length of order list (song length)
    uint16_t num_samples;                    // Number of samples/instruments
    uint16_t current_pattern;                // Currently playing pattern
    uint16_t current_row;                    // Currently playing row
    uint16_t current_order;                  // Current position in order list
    uint16_t rows_per_pattern;               // Rows in current pattern (can vary per pattern)
    uint32_t total_rows;                     // Total rows for channel-based formats
    uint16_t rows_per_beat;                  // Rows per beat (for timeline display)
    uint16_t rows_per_measure;               // Rows per measure (for timeline display)
    RVChannelInfo channels[RV_MAX_CHANNELS]; // Per-channel info
    char song_name[64];                      // Song title
    char game_name[64];                      // Game/album name
    char artist_name[64];                    // Artist/composer name
    char sample_names[32][24];               // Sample/instrument names (up to 32)

    // For formats with per-channel scrolling (VGM, etc.)
    uint32_t current_sample;   // Current playback position in samples
    void* native_pattern_data; // Pointer to format-specific pattern data (e.g., VgmPattern*)

    // Module type identifier from the playback plugin (e.g., "mod", "xm", "s3m", "it")
    char module_type[16];
} RVTrackerInfo;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct RVPlaybackPlugin {
    // Version of the API. This has to be set to RV_PLAYBACK_PLUGIN_API_VERSION
    uint64_t api_version;
    // Name of the plugin. This name should be unique or it may fail to load if there is a collision.
    const char* name;
    // Scematic version of the plugin. If the version doesn't follow the rules of SemVersion it may fail to load.
    const char* version;
    // Scematic version of library being used. Useful if you only update lib x from 0.1 to 0.2 but no other changes to
    // plugin In case the plugin doesn't use any external library this can be set to "" or NULL
    const char* library_version;
    // Ask the plugin if it can play some data. The plugin has to determine from the header if it supports the
    // file or not. The input data is at least 2048 bytes but can be less if the the total file is smaller.
    // Notice that no user data is provided with this as the plugin instance hasn't actually been created
    // The plugin must support to parse this data without custom data being setup.
    // It's encouraged that plugins detect the song-type based on metadata, but the filename is included
    // in case that is the only option to detect support for the file type
    RVProbeResult (*probe_can_play)(uint8_t* data, uint64_t data_size, const char* filename, uint64_t total_size);
    // Returns a comma separated list of supported extensions
    const char* (*supported_extensions)(void);
    void* (*create)(const RVService* services);
    // Destroy the instance of the plugin. It's expected that the user will free the user_data pointer at
    // this point as it won't be used anymore.
    int (*destroy)(void* user_data);
    // TODO
    void (*event)(void* user_data, uint8_t* data, uint64_t data_size);
    // Opens a buffer to be ready for playback. Buffer may be a file/archived/file or a file or a network resource.
    // Use the RVFileAPI that can be optained from services to load the data
    int (*open)(void* user_data, const char* url, uint32_t subsong, const RVService* services);
    // Closes the file buffer that was opened in open. Notice that the plugin isn't detroyed at this but but is
    // here for closing an open file/stream/etc
    void (*close)(void* user_data);
    // Called when sample data is requested from the host
    // The plugin is allowed to return as many samples as it want's as long as it doesn't go above max sample count
    RVReadInfo (*read_data)(void* user_data, RVReadData dest);
    // Called requesting a new location in the data
    int64_t (*seek)(void* user_data, int64_t ms);
    // Called to see if the plugin can provide some metadata given an url
    int (*metadata)(const char* url, const RVService* services);
    // Called once for each plugin. This allows the plugin to setup an instance of the logging api
    void (*static_init)(const RVService* services);
    // Called when the user has changed some settings
    RVSettingsUpdate (*settings_updated)(void* user_data, const RVService* services);

    // --- Tracker visualization API (optional, set to NULL if not supported) ---

    // Get tracker module information. Returns 0 on success, non-zero if not a tracker format.
    // This provides song structure (num patterns, channels, samples, names, etc.)
    int (*get_tracker_info)(void* user_data, RVTrackerInfo* info);

    // Get a single pattern cell. Returns 0 on success, non-zero on error.
    // pattern: pattern number (0 to num_patterns-1)
    // row: row number (0 to rows_per_pattern-1)
    // channel: channel number (0 to num_channels-1)
    // cell: output cell data
    int (*get_pattern_cell)(void* user_data, int pattern, int row, int channel, RVPatternCell* cell);

    // Get number of rows in a specific pattern (patterns can have different lengths)
    // For channel-based formats (TFMX, etc.), use pattern=0 and this returns total_rows
    // Returns 0 if pattern index is invalid
    int (*get_pattern_num_rows)(void* user_data, int pattern);

    // --- Scope/Waveform visualization API (optional, set to NULL if not supported) ---

    // Get per-channel audio scope data for visualization.
    // When first called, the plugin automatically enables scope capture internally.
    // channel: channel number (0 to num_channels-1)
    // buffer: output buffer for float samples [-1.0, 1.0]
    // num_samples: maximum number of samples to retrieve
    // Returns actual number of samples written to buffer (0 if not supported)
    uint32_t (*get_scope_data)(void* user_data, int channel, float* buffer, uint32_t num_samples);

    // --- Cleanup API (optional, set to NULL if not needed) ---

    // Called once per plugin during shutdown, before the library is unloaded.
    // Use this to clean up resources allocated in static_init (e.g., library shutdown).
    // If NULL, no cleanup is performed.
    void (*static_destroy)(void);

    // --- Scope channel names API (optional, set to NULL if not supported) ---

    // Get scope channel names and count for the currently open song.
    // Fills names[] with pointers to plugin-owned strings (valid until close/destroy).
    // Returns number of scope channels (0 if not supported).
    uint32_t (*get_scope_channel_names)(void* user_data, const char** names, uint32_t max_channels);
} RVPlaybackPlugin;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Entry point signature for playback plugins
typedef RVPlaybackPlugin* (*RVPlaybackPluginGetFunc)(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
