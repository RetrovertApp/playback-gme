///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GME Playback Plugin
//
// Implements RVPlaybackPlugin interface for classic video game music formats using game-music-emu.
// Supported formats: SPC, NSF, NSFE, GBS, VGM, VGZ, AY, GYM, HES, KSS
// Note: SAP format is handled by the ASAP plugin instead (Atari 8-bit SAP is different from GME's SAP)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// C11 nullptr compatibility
#ifndef nullptr
#define nullptr ((void*)0)
#endif

#include <retrovert/io.h>
#include <retrovert/log.h>
#include <retrovert/metadata.h>
#include <retrovert/playback.h>
#include <retrovert/service.h>

#include "gme.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define OUTPUT_SAMPLE_RATE 48000

RV_PLUGIN_USE_IO_API();
RV_PLUGIN_USE_METADATA_API();
RV_PLUGIN_USE_LOG_API();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct GmeReplayerData {
    Music_Emu* emu;
    int current_track;
    int track_count;
} GmeReplayerData;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const char* gme_plugin_supported_extensions(void) {
    return "spc,nsf,nsfe,gbs,vgm,vgz,ay,gym,hes,kss";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void* gme_plugin_create(const RVService* service_api) {
    GmeReplayerData* data = malloc(sizeof(GmeReplayerData));
    if (data == nullptr) {
        return nullptr;
    }
    memset(data, 0, sizeof(GmeReplayerData));

    return data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int gme_plugin_destroy(void* user_data) {
    GmeReplayerData* data = (GmeReplayerData*)user_data;

    if (data->emu) {
        gme_delete(data->emu);
    }

    free(data);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int gme_plugin_open(void* user_data, const char* url, uint32_t subsong, const RVService* service_api) {
    (void)service_api;

    RVIoReadUrlResult read_res;

    if ((read_res = rv_io_read_url_to_memory(url)).data == nullptr) {
        rv_error("Failed to load %s to memory", url);
        return -1;
    }

    GmeReplayerData* data = (GmeReplayerData*)user_data;

    // Free previous emulator if any
    if (data->emu) {
        gme_delete(data->emu);
        data->emu = nullptr;
    }

    // Open data from memory
    gme_err_t err = gme_open_data(read_res.data, (long)read_res.data_size, &data->emu, OUTPUT_SAMPLE_RATE);
    if (err != nullptr) {
        rv_error("GME failed to open %s: %s", url, err);
        rv_io_free_url_to_memory(read_res.data);
        return -1;
    }

    data->track_count = gme_track_count(data->emu);

    // Clamp subsong to valid range
    if ((int)subsong >= data->track_count) {
        subsong = 0;
    }
    data->current_track = (int)subsong;

    // Start the track
    err = gme_start_track(data->emu, data->current_track);
    if (err != nullptr) {
        rv_error("GME failed to start track %d of %s: %s", data->current_track, url, err);
        gme_delete(data->emu);
        data->emu = nullptr;
        rv_io_free_url_to_memory(read_res.data);
        return -1;
    }

    // Set fade for tracks without duration info (default 2.5 minutes + 8 second fade)
    gme_info_t* track_info = nullptr;
    err = gme_track_info(data->emu, &track_info, data->current_track);
    if (err == nullptr && track_info != nullptr) {
        if (track_info->length > 0) {
            gme_set_fade(data->emu, track_info->length);
        } else if (track_info->play_length > 0) {
            gme_set_fade(data->emu, track_info->play_length);
        }
        gme_free_info(track_info);
    }

    rv_io_free_url_to_memory(read_res.data);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void gme_plugin_close(void* user_data) {
    GmeReplayerData* data = (GmeReplayerData*)user_data;

    if (data->emu) {
        gme_delete(data->emu);
        data->emu = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Helper to check if an extension is in our supported list
static bool gme_is_supported_ext(const char* ext) {
    if (ext == nullptr || ext[0] == '\0') {
        return false;
    }
    // Our supported extensions (must match gme_plugin_supported_extensions)
    static const char* supported[] = { "spc", "nsf", "nsfe", "gbs", "vgm", "vgz", "ay", "gym", "hes", "kss", nullptr };
    for (int i = 0; supported[i] != nullptr; i++) {
        if (strcasecmp(ext, supported[i]) == 0) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static RVProbeResult gme_plugin_probe_can_play(uint8_t* probe_data, uint64_t data_size, const char* url,
                                               uint64_t total_size) {
    (void)url;
    (void)total_size;

    // Need at least 4 bytes to check magic
    if (data_size < 4) {
        return RVProbeResult_Unsupported;
    }

    // Use GME's built-in header identification
    // IMPORTANT: gme_identify_header may return formats we don't support (e.g., "SAP" for Atari 8-bit)
    // so we must verify the extension is in our supported list
    const char* ext = gme_identify_header(probe_data);
    if (ext != nullptr && ext[0] != '\0' && gme_is_supported_ext(ext)) {
        // For VGM/VGZ/GYM, return Unsure so libvgm plugin gets priority
        // libvgm has more complete support (better chip emulation, VGZ decompression)
        if (strcasecmp(ext, "vgm") == 0 || strcasecmp(ext, "vgz") == 0 || strcasecmp(ext, "gym") == 0) {
            return RVProbeResult_Unsure;
        }
        return RVProbeResult_Supported;
    }

    // Check specific magic bytes for formats that might not be detected by gme_identify_header

    // SPC: "SNES-SPC700 Sound File Data" at offset 0
    if (data_size >= 27) {
        if (memcmp(probe_data, "SNES-SPC700 Sound File Data", 27) == 0) {
            return RVProbeResult_Supported;
        }
    }

    // NSF: "NESM" + 0x1A
    if (data_size >= 5) {
        if (probe_data[0] == 'N' && probe_data[1] == 'E' && probe_data[2] == 'S' && probe_data[3] == 'M'
            && probe_data[4] == 0x1A) {
            return RVProbeResult_Supported;
        }
    }

    // NSFE: "NSFE"
    if (probe_data[0] == 'N' && probe_data[1] == 'S' && probe_data[2] == 'F' && probe_data[3] == 'E') {
        return RVProbeResult_Supported;
    }

    // GBS: "GBS" + 0x01
    if (probe_data[0] == 'G' && probe_data[1] == 'B' && probe_data[2] == 'S' && probe_data[3] == 0x01) {
        return RVProbeResult_Supported;
    }

    // VGM: "Vgm " (0x56 0x67 0x6D 0x20)
    // Return Unsure so libvgm plugin gets priority
    if (probe_data[0] == 'V' && probe_data[1] == 'g' && probe_data[2] == 'm' && probe_data[3] == ' ') {
        return RVProbeResult_Unsure;
    }

    // VGZ: Gzip magic (0x1F 0x8B)
    // Return Unsure so libvgm plugin gets priority
    if (probe_data[0] == 0x1F && probe_data[1] == 0x8B) {
        // Could be VGZ - check extension via url if available
        if (url != nullptr) {
            const char* dot = strrchr(url, '.');
            if (dot != nullptr && (strcasecmp(dot, ".vgz") == 0 || strcasecmp(dot, ".vgm.gz") == 0)) {
                return RVProbeResult_Unsure;
            }
        }
        return RVProbeResult_Unsure;
    }

    // AY: "ZXAYEMUL"
    if (data_size >= 8) {
        if (memcmp(probe_data, "ZXAYEMUL", 8) == 0) {
            return RVProbeResult_Supported;
        }
    }

    // GYM: "GYMX"
    // Return Unsure so libvgm plugin gets priority
    if (probe_data[0] == 'G' && probe_data[1] == 'Y' && probe_data[2] == 'M' && probe_data[3] == 'X') {
        return RVProbeResult_Unsure;
    }

    // HES: Check for HES header (0x48 0x45 0x53 0x4D = "HESM")
    if (probe_data[0] == 0x48 && probe_data[1] == 0x45 && probe_data[2] == 0x53 && probe_data[3] == 0x4D) {
        return RVProbeResult_Supported;
    }

    // KSS: "KSCC" or "KSSX"
    if (probe_data[0] == 'K' && probe_data[1] == 'S' && probe_data[2] == 'S'
        && (probe_data[3] == 'C' || probe_data[3] == 'X')) {
        return RVProbeResult_Supported;
    }

    return RVProbeResult_Unsupported;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static RVReadInfo gme_plugin_read_data(void* user_data, RVReadData dest) {
    GmeReplayerData* data = (GmeReplayerData*)user_data;
    RVAudioFormat format = { RVAudioStreamFormat_S16, 2, OUTPUT_SAMPLE_RATE };

    if (data->emu == nullptr) {
        return (RVReadInfo) { format, 0, RVReadStatus_Error};
    }

    if (gme_track_ended(data->emu)) {
        return (RVReadInfo) { format, 0, RVReadStatus_Finished};
    }

    // Calculate how many S16 stereo frames fit in the output buffer
    uint32_t max_frames = dest.channels_output_max_bytes_size / (sizeof(int16_t) * 2);

    // GME outputs stereo S16 samples directly to output buffer
    int sample_count = (int)max_frames * 2;
    gme_err_t err = gme_play(data->emu, sample_count, (int16_t*)dest.channels_output);
    if (err != nullptr) {
        rv_error("GME playback error: %s", err);
        return (RVReadInfo) { format, 0, RVReadStatus_Error};
    }

    RVReadStatus status = gme_track_ended(data->emu) ? RVReadStatus_Finished : RVReadStatus_Ok;
    return (RVReadInfo) { format, (uint16_t)max_frames, status};
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int64_t gme_plugin_seek(void* user_data, int64_t ms) {
    GmeReplayerData* data = (GmeReplayerData*)user_data;

    if (data->emu == nullptr) {
        return -1;
    }

    gme_err_t err = gme_seek(data->emu, (int)ms);
    if (err != nullptr) {
        rv_error("GME seek error: %s", err);
        return -1;
    }

    return ms;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int gme_plugin_metadata(const char* url, const RVService* service_api) {
    (void)service_api;
    RVIoReadUrlResult read_res;

    if ((read_res = rv_io_read_url_to_memory(url)).data == nullptr) {
        rv_error("Failed to load %s to memory", url);
        return -1;
    }

    Music_Emu* emu = nullptr;
    gme_err_t err = gme_open_data(read_res.data, (long)read_res.data_size, &emu, gme_info_only);
    if (err != nullptr) {
        rv_io_free_url_to_memory(read_res.data);
        return -1;
    }

    RVMetadataId index = rv_metadata_create_url(url);

    // Get track info for the first track
    gme_info_t* track_info = nullptr;
    err = gme_track_info(emu, &track_info, 0);
    if (err == nullptr && track_info != nullptr) {
        // Set title
        if (track_info->song != nullptr && track_info->song[0] != '\0') {
            rv_metadata_set_tag(index, RV_METADATA_TITLE_TAG, track_info->song);
        } else if (track_info->game != nullptr && track_info->game[0] != '\0') {
            rv_metadata_set_tag(index, RV_METADATA_TITLE_TAG, track_info->game);
        }

        // Set artist/author
        if (track_info->author != nullptr && track_info->author[0] != '\0') {
            rv_metadata_set_tag(index, RV_METADATA_ARTIST_TAG, track_info->author);
        }

        // Set system as song type
        if (track_info->system != nullptr && track_info->system[0] != '\0') {
            rv_metadata_set_tag(index, RV_METADATA_SONGTYPE_TAG, track_info->system);
        }

        // Set duration
        if (track_info->play_length > 0) {
            rv_metadata_set_tag_f64(index, RV_METADATA_LENGTH_TAG, track_info->play_length / 1000.0);
        } else if (track_info->length > 0) {
            rv_metadata_set_tag_f64(index, RV_METADATA_LENGTH_TAG, track_info->length / 1000.0);
        }

        gme_free_info(track_info);
    }

    // Add subsongs if more than one track
    int track_count = gme_track_count(emu);
    if (track_count > 1) {
        for (int i = 0; i < track_count; i++) {
            gme_info_t* subsong_info = nullptr;
            err = gme_track_info(emu, &subsong_info, i);
            if (err == nullptr && subsong_info != nullptr) {
                const char* subsong_name = "";
                if (subsong_info->song != nullptr && subsong_info->song[0] != '\0') {
                    subsong_name = subsong_info->song;
                }
                float length = 0.0f;
                if (subsong_info->play_length > 0) {
                    length = subsong_info->play_length / 1000.0f;
                } else if (subsong_info->length > 0) {
                    length = subsong_info->length / 1000.0f;
                }
                rv_metadata_add_subsong(index, i, subsong_name, length);
                gme_free_info(subsong_info);
            } else {
                rv_metadata_add_subsong(index, i, "", 0.0f);
            }
        }
    }

    gme_delete(emu);
    rv_io_free_url_to_memory(read_res.data);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void gme_plugin_event(void* user_data, uint8_t* event_data, uint64_t len) {
    GmeReplayerData* data = (GmeReplayerData*)user_data;

    if (data->emu == nullptr || event_data == nullptr || len < 8) {
        return;
    }

    // Position info - GME doesn't expose detailed position, just current time
    int current_ms = gme_tell(data->emu);
    event_data[7] = (uint8_t)((current_ms / 1000) & 0xFF);
    event_data[6] = (uint8_t)((current_ms / 100) % 10);
    event_data[5] = 0;
    event_data[4] = 0;

    // GME doesn't expose per-channel volume info, so set VU meters to zero
    event_data[3] = 0;
    event_data[2] = 0;
    event_data[1] = 0;
    event_data[0] = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void gme_plugin_static_init(const RVService* service_api) {
    rv_init_log_api(service_api);
    rv_init_io_api(service_api);
    rv_init_metadata_api(service_api);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static RVPlaybackPlugin g_gme_plugin = {
    RV_PLAYBACK_PLUGIN_API_VERSION,
    "gme",
    "0.0.1",
    "game-music-emu 0.6.4",
    gme_plugin_probe_can_play,
    gme_plugin_supported_extensions,
    gme_plugin_create,
    gme_plugin_destroy,
    gme_plugin_event,
    gme_plugin_open,
    gme_plugin_close,
    gme_plugin_read_data,
    gme_plugin_seek,
    gme_plugin_metadata,
    gme_plugin_static_init,
    nullptr, // settings_updated
    nullptr, // get_tracker_info (not a tracker format)
    nullptr, // get_pattern_cell
    nullptr, // get_pattern_num_rows
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RV_EXPORT RVPlaybackPlugin* rv_playback_plugin(void) {
    return &g_gme_plugin;
}
