# GME Playback Plugin
#
# This plugin plays classic video game music formats using game-music-emu library:
# - SPC (Super Nintendo)
# - NSF/NSFE (NES/Famicom)
# - GBS (Game Boy)
# - VGM/VGZ (Sega Genesis, Master System, Game Gear)
# - AY (ZX Spectrum, Amstrad CPC)
# - GYM (Sega Genesis)
# - HES (PC Engine/TurboGrafx-16)
# - KSS (MSX, Master System)

cmake_minimum_required(VERSION 3.16)
project(playback_gme C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PlaybackPlugin)

# Download game-music-emu 0.6.4
download_library(
    NAME gme
    URL "https://github.com/libgme/game-music-emu/archive/refs/tags/0.6.4.tar.gz"
    FILENAME "game-music-emu-0.6.4.tar.gz"
    HASH "f2360feb5a32ace226c583df4faf6eff74145c81264aaea11e17a1af2f6f101a"
    EXTRACT_DIR "game-music-emu-0.6.4"
)

# Build gme as a static library to embed in our plugin
set(GME_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(GME_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(GME_BUILD_FRAMEWORK OFF CACHE BOOL "" FORCE)
set(GME_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(GME_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GME_ZLIB OFF CACHE BOOL "" FORCE)  # Disable zlib to simplify the build

# Enable PIC for the static library since we link it into a shared library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Add the gme library from downloaded source
add_subdirectory("${gme_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/gme-build" EXCLUDE_FROM_ALL)

# Make sure gme_static has PIC enabled
set_target_properties(gme_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Strip GCC-style flags that GME's upstream CMake sets but clang-cl rejects
if(MSVC)
    get_target_property(gme_opts gme_static COMPILE_OPTIONS)
    if(gme_opts)
        list(REMOVE_ITEM gme_opts "-fno-rtti" "-fno-exceptions")
        set_target_properties(gme_static PROPERTIES COMPILE_OPTIONS "${gme_opts}")
    endif()
endif()
suppress_external_warnings(gme_static)

add_library(gme_playback SHARED
    gme_plugin.c
)

target_include_directories(gme_playback PRIVATE
    ${gme_SOURCE_DIR}/gme
)

# Link with gme static library
# GME is C++, so we need to link the C++ standard library even though our plugin is C.
# On Windows the C++ runtime is linked automatically via MSVC's CRT.
target_link_libraries(gme_playback PRIVATE gme_static)
if(NOT WIN32)
    target_link_libraries(gme_playback PRIVATE stdc++)
endif()

target_compile_options(gme_playback PRIVATE
    -Wno-unused-parameter
    -Wno-sign-compare
)

rv_set_plugin_properties(gme_playback)
