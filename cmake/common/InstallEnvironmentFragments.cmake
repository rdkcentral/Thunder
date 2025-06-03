# /**
#  * InstallEnvironmentFragments
#  *
#  * This function processes and installs environment and sysconf fragment files
#  * from a specified directory or a default scan path. It supports filtering,
#  * masking, and configuring files before installation.
#  *
#  * Options:
#  *   - NO_SYSCONF: Exclude sysconf files (*.sysconf.in) from processing.
#  *   - NO_ENV: Exclude environment files (*.env.in) from processing.
#  *
#  * Arguments:
#  *   - SCAN_PATH: (Optional) Directory to scan for input files. Defaults to the
#  *                directory containing this CMake script.
#  *   - INSTALL_PATH: (Optional) Destination directory for the installed files.
#  *                   Defaults to `${CMAKE_INSTALL_FULL_SYSCONFDIR}/${NAMESPACE}/environment.d`
#  *                   or `${CONFIG_INSTALL_PATH}/environment.d` if defined.
#  *   - MASKS: (Optional) List of filename patterns to exclude from processing.
#  *            Patterns can include wildcards (*).
#  *   - FILES: (Optional) List of specific files to process. If provided, only
#  *            these files will be processed.
#  *
#  * Behavior:
#  *   - If both NO_ENV and NO_SYSCONF are set, a warning is issued, and no files
#  *     are processed.
#  *   - If no files are found to process, a warning is issued, and the function
#  *     exits without installing anything.
#  *   - Input files are configured using `configure_file` and installed to the
#  *     specified or default destination with appropriate permissions.
#  *
#  * Example Usage:
#  *   include(InstallEnvironmentFragments)
#  *   
#  *   InstallEnvironmentFragments(
#  *       SCAN_PATH "/path/to/scan"
#  *       INSTALL_PATH "/path/to/install"
#  *       MASKS "*.exclude"
#  *       FILES "specific.env.in"
#  *       NO_ENV
#  *   )
#  */

function(InstallEnvironmentFragments)
    set(options NO_SYSCONF NO_ENV)  # e.g., options like OPTIONAL
    set(oneValueArgs SCAN_PATH INSTALL_PATH)
    set(multiValueArgs MASKS FILES)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_SCAN_DIR)
        set(_scan_path "${CMAKE_CURRENT_LIST_DIR}")
    else()
        set(_scan_path "${ARG_SCAN_DIR}")
    endif()

    if(NOT EXISTS "${_scan_path}")
        message(SEND_ERROR "Scan directory '${_scan_path}' does not exist.")
        return()
    endif() 

    if(NOT ARG_INSTALL_PATH)
        if (DEFINED CONFIG_INSTALL_PATH)
            set(_base_install_path "${CONFIG_INSTALL_PATH}")
        else()
            set(_base_install_path "${CMAKE_INSTALL_FULL_SYSCONFDIR}/${NAMESPACE}")
        endif()

        set(_install_path "${_base_install_path}/environment.d")
    else()
        set(_install_path "${ARG_INSTALL_PATH}")
    endif(NOT ARG_INSTALL_PATH)

    if(ARG_FILES)
        set(ENVIRONMENT_FRAGMENT_INPUTS "")
        foreach(_file ${ARG_FILES})
            if(IS_ABSOLUTE "${_file}")
                list(APPEND ENVIRONMENT_FRAGMENT_INPUTS "${_file}")
            else()
                list(APPEND ENVIRONMENT_FRAGMENT_INPUTS "${_scan_path}/${_file}")
            endif()
        endforeach()
    else()
        if(NOT ARG_NO_ENV)
            set(ENV_FRAGMENTS_FILTER "${_scan_path}/*.env.in")
        endif()

        if(NOT ARG_NO_SYSCONF)
            set(SYSCONF_FRAGMENTS_FILTER "${_scan_path}/*.sysconf.in")
        endif()

        if(ARG_NO_ENV AND ARG_NO_SYSCONF)
            message(AUTHOR_WARNING "Both NO_ENV and NO_SYSCONF are set; no environment or sysconf files will be installed.")
            return()
        endif()

        file(GLOB ENVIRONMENT_FRAGMENT_INPUTS
            "${SYSCONF_FRAGMENTS_FILTER}"
            "${ENV_FRAGMENTS_FILTER}"
        )
    endif()

    # Remove files matching any mask
    if(ARG_MASKS)
        set(_filtered_inputs "")
        foreach(_file ${ENVIRONMENT_FRAGMENT_INPUTS})
            set(_exclude FALSE)
            foreach(_mask ${ARG_MASKS})
                # Convert mask to regex: escape . and replace * with .*
                string(REPLACE "." "\\." _mask_regex "${_mask}")
                string(REPLACE "*" ".*" _mask_regex "${_mask_regex}")
                get_filename_component(_filename "${_file}" NAME)
                if(_filename MATCHES "^${_mask_regex}$")
                    set(_exclude TRUE)
                endif()
            endforeach()
            if(NOT _exclude)
                list(APPEND _filtered_inputs "${_file}")
            endif()
        endforeach()
        set(ENVIRONMENT_FRAGMENT_INPUTS ${_filtered_inputs})
    endif()

    if (NOT ENVIRONMENT_FRAGMENT_INPUTS)
        message(AUTHOR_WARNING "No environment (.env.in) or sysconf (.sysconf.in) files were found to process in '${_scan_path}'. Nothing will be installed.")
        return()
    endif()

    foreach(INPUT_FILE ${ENVIRONMENT_FRAGMENT_INPUTS})
        get_filename_component(FILENAME ${INPUT_FILE} NAME)
        string(REGEX REPLACE "\\.in$" "" OUTPUT_FILENAME "${FILENAME}")
        set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/${OUTPUT_FILENAME}")

        configure_file(
            "${INPUT_FILE}"
            "${OUTPUT_FILE}"
            @ONLY
        )

        install(
            FILES "${OUTPUT_FILE}"
            DESTINATION "${_install_path}"
            PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
            COMPONENT ${NAMESPACE}_Runtime
        )

        message(STATUS "Processed environment fragment: ${INPUT_FILE} -> ${_install_path}/${OUTPUT_FILENAME}")
    endforeach()
endfunction()