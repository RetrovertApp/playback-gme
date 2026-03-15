# PlaybackPlugin.cmake
#
# Common setup for Retrovert playback plugins.
# Include this from a plugin's CMakeLists.txt to get sensible defaults.
#
# Sets:
#   RETROVERT_INCLUDE_DIR     - path to vendored API headers
#   RETROVERT_PLUGIN_OUTPUT_DIR - where built plugins go
#   PLAYBACK_PLUGIN_SUFFIX    - .so or .dll
#
# Also includes DependencyHelpers.cmake (download_library, suppress_external_warnings).

include_guard()

# Include dependency helpers from the same cmake/ directory
include(DependencyHelpers)

# API headers — vendored in each plugin repo
if(NOT RETROVERT_INCLUDE_DIR)
    set(RETROVERT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

# Output directory for built plugin shared libraries
if(NOT RETROVERT_PLUGIN_OUTPUT_DIR)
    set(RETROVERT_PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")
endif()

# Plugin file suffix
if(NOT PLAYBACK_PLUGIN_SUFFIX)
    if(WIN32)
        set(PLAYBACK_PLUGIN_SUFFIX ".dll")
    else()
        set(PLAYBACK_PLUGIN_SUFFIX ".so")
    endif()
endif()

# Sanitizer support (standalone builds)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

################################################################################
# rv_set_plugin_properties(<target>)
#
# Apply standard properties to a playback plugin target:
#   - Suppresses compiler warnings (third-party code)
#   - Sets output directory, prefix, suffix
#   - Adds RETROVERT_INCLUDE_DIR to include path
#   - Adds sanitizer flags if enabled
#   - Adds install rules
################################################################################
function(rv_set_plugin_properties target_name)
    suppress_external_warnings(${target_name})

    target_include_directories(${target_name} PRIVATE ${RETROVERT_INCLUDE_DIR})

    set_target_properties(${target_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${RETROVERT_PLUGIN_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${RETROVERT_PLUGIN_OUTPUT_DIR}"
        PREFIX ""
        SUFFIX "${PLAYBACK_PLUGIN_SUFFIX}"
    )

    if(ENABLE_ASAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address)
    endif()
    if(ENABLE_TSAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=thread)
        target_link_options(${target_name} PRIVATE -fsanitize=thread)
    endif()

    install(TARGETS ${target_name} LIBRARY DESTINATION plugins RUNTIME DESTINATION plugins)
endfunction()
