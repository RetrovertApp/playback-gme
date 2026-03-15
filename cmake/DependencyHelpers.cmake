# Dependency Helper Functions
#
# This module provides helper functions for downloading, caching, and patching
# third-party library sources used by playback plugins and other components.

################################################################################
# download_library()
#
# Downloads a library tarball, extracts it, and optionally applies patches.
# Designed for replacing vendored library sources with on-demand downloads.
#
# Usage:
#   download_library(
#       NAME <name>                    # Library identifier (used for paths and variables)
#       URL <url>                      # Tarball download URL
#       FILENAME <filename>            # Local filename for the cached tarball
#       HASH <sha256>                  # SHA-256 hash for integrity verification
#       [EXTRACT_DIR <dir>]            # Directory name inside the tarball (default: NAME)
#       [PATCHES <p1> <p2>...]         # Patch files to apply in order (relative to caller)
#   )
#
# Behavior:
#   1. Downloads tarball to ${CMAKE_SOURCE_DIR}/.deps/ (source-tree cache, survives clean rebuilds)
#   2. Extracts to ${CMAKE_BINARY_DIR}/_deps/${NAME}/ (build-tree, per-config)
#   3. Applies patches using "git apply" if PATCHES specified
#   4. Writes stamp file to skip re-extraction on subsequent configures
#   5. Sets ${NAME}_SOURCE_DIR in PARENT_SCOPE pointing to extracted source
#
################################################################################
function(download_library)
    # Parse arguments
    set(options "")
    set(oneValueArgs NAME URL FILENAME HASH EXTRACT_DIR)
    set(multiValueArgs PATCHES)
    cmake_parse_arguments(DL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT DL_NAME)
        message(FATAL_ERROR "download_library: NAME is required")
    endif()
    if(NOT DL_URL)
        message(FATAL_ERROR "download_library: URL is required")
    endif()
    if(NOT DL_FILENAME)
        message(FATAL_ERROR "download_library: FILENAME is required")
    endif()
    if(NOT DL_HASH)
        message(FATAL_ERROR "download_library: HASH is required")
    endif()

    # Default EXTRACT_DIR to NAME if not specified
    if(NOT DL_EXTRACT_DIR)
        set(DL_EXTRACT_DIR "${DL_NAME}")
    endif()

    # Paths - allow CI to override cache directory via environment variable
    if(DEFINED ENV{RP_DEPS_CACHE_DIR})
        set(deps_cache_dir "$ENV{RP_DEPS_CACHE_DIR}")
    else()
        set(deps_cache_dir "${CMAKE_SOURCE_DIR}/.deps")
    endif()
    set(tarball_path "${deps_cache_dir}/${DL_FILENAME}")
    set(extract_base "${CMAKE_BINARY_DIR}/_deps/${DL_NAME}")
    set(source_dir "${extract_base}/${DL_EXTRACT_DIR}")
    set(stamp_file "${extract_base}/.stamp")

    # Compute stamp content (hash of library hash + patch files)
    # If any patch changes, the stamp changes and we re-extract + re-patch
    set(stamp_content "${DL_HASH}")
    foreach(patch_file ${DL_PATCHES})
        # Resolve patch path relative to the calling CMakeLists.txt
        if(NOT IS_ABSOLUTE "${patch_file}")
            set(patch_file "${CMAKE_CURRENT_SOURCE_DIR}/${patch_file}")
        endif()
        if(EXISTS "${patch_file}")
            file(SHA256 "${patch_file}" patch_hash)
            string(APPEND stamp_content ":${patch_hash}")
        else()
            message(FATAL_ERROR "download_library(${DL_NAME}): Patch file not found: ${patch_file}")
        endif()
    endforeach()
    string(SHA256 stamp_hash "${stamp_content}")

    # Check if we can skip extraction (stamp matches)
    set(needs_extract TRUE)
    if(EXISTS "${stamp_file}")
        file(READ "${stamp_file}" existing_stamp)
        string(STRIP "${existing_stamp}" existing_stamp)
        if("${existing_stamp}" STREQUAL "${stamp_hash}")
            set(needs_extract FALSE)
        endif()
    endif()

    if(needs_extract)
        # Ensure cache directory exists
        file(MAKE_DIRECTORY "${deps_cache_dir}")

        # Download tarball if not cached (or hash mismatch)
        set(needs_download TRUE)
        if(EXISTS "${tarball_path}")
            file(SHA256 "${tarball_path}" existing_hash)
            if("${existing_hash}" STREQUAL "${DL_HASH}")
                set(needs_download FALSE)
            else()
                message(STATUS "[${DL_NAME}] Cached tarball hash mismatch, re-downloading")
                file(REMOVE "${tarball_path}")
            endif()
        endif()

        if(needs_download)
            message(STATUS "[${DL_NAME}] Downloading ${DL_URL}")
            file(DOWNLOAD
                "${DL_URL}"
                "${tarball_path}"
                EXPECTED_HASH SHA256=${DL_HASH}
                STATUS download_status
                SHOW_PROGRESS
            )
            list(GET download_status 0 download_code)
            list(GET download_status 1 download_message)
            if(NOT download_code EQUAL 0)
                file(REMOVE "${tarball_path}")
                message(FATAL_ERROR "[${DL_NAME}] Download failed: ${download_message}")
            endif()
        endif()

        # Clean and extract
        if(EXISTS "${extract_base}")
            file(REMOVE_RECURSE "${extract_base}")
        endif()
        file(MAKE_DIRECTORY "${extract_base}")

        message(STATUS "[${DL_NAME}] Extracting ${DL_FILENAME}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xf "${tarball_path}"
            WORKING_DIRECTORY "${extract_base}"
            RESULT_VARIABLE extract_result
        )
        if(NOT extract_result EQUAL 0)
            message(FATAL_ERROR "[${DL_NAME}] Extraction failed (exit code ${extract_result})")
        endif()

        # Verify source directory exists after extraction
        if(NOT EXISTS "${source_dir}")
            # List what was extracted to help debug
            file(GLOB extracted_dirs "${extract_base}/*")
            message(FATAL_ERROR "[${DL_NAME}] Expected directory '${DL_EXTRACT_DIR}' not found after extraction. Found: ${extracted_dirs}")
        endif()

        # Apply patches
        foreach(patch_file ${DL_PATCHES})
            if(NOT IS_ABSOLUTE "${patch_file}")
                set(patch_file "${CMAKE_CURRENT_SOURCE_DIR}/${patch_file}")
            endif()
            get_filename_component(patch_name "${patch_file}" NAME)
            message(STATUS "[${DL_NAME}] Applying patch: ${patch_name}")
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E env GIT_DIR= git apply --whitespace=nowarn --ignore-whitespace "${patch_file}"
                WORKING_DIRECTORY "${source_dir}"
                RESULT_VARIABLE patch_result
                ERROR_VARIABLE patch_error
            )
            if(NOT patch_result EQUAL 0)
                message(FATAL_ERROR "[${DL_NAME}] Failed to apply patch ${patch_name}: ${patch_error}")
            endif()
        endforeach()

        # Write stamp file
        file(WRITE "${stamp_file}" "${stamp_hash}\n")
        message(STATUS "[${DL_NAME}] Ready at ${source_dir}")
    else()
        message(STATUS "[${DL_NAME}] Using cached extraction at ${source_dir}")
    endif()

    # Export source directory to caller
    set(${DL_NAME}_SOURCE_DIR "${source_dir}" PARENT_SCOPE)
endfunction()

################################################################################
# suppress_external_warnings(<target>)
#
# Suppresses compiler warnings for third-party library targets.
# On MSVC/clang-cl, also strips GCC-style -W flags that clang-cl does not
# recognize (semantic flags like -funsigned-char are preserved).
#
# Use on targets that compile external source code we don't maintain.
################################################################################
function(suppress_external_warnings target)
    if(MSVC)
        # clang-cl does not recognize GCC/Clang-style -W flags.
        # Strip them to avoid "unknown argument" warnings from the driver.
        get_target_property(opts ${target} COMPILE_OPTIONS)
        if(opts)
            list(FILTER opts EXCLUDE REGEX "^-W")
            set_target_properties(${target} PROPERTIES COMPILE_OPTIONS "${opts}")
        endif()
        target_compile_options(${target} PRIVATE /w)
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
endfunction()
