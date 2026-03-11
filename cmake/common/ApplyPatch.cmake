function(GetPatchMarker MARKER SOURCE_DIR PATCH_FILE )
    string(MD5 HASH "${SOURCE_DIR}::${PATCH_FILE}")
    get_filename_component(PATCH_NAME "${PATCH_FILE}" NAME_WE)
    set(${MARKER} "${CMAKE_CURRENT_BINARY_DIR}/.patch-${PATCH_NAME}-${HASH}" PARENT_SCOPE)
endfunction()

function(ApplyPatch SOURCE_DIR PATCH_FILE)
    GetPatchMarker(PATCH_APPLIED ${SOURCE_DIR} ${PATCH_FILE})
    
    # Find git
    if (NOT EXISTS "${PATCH_APPLIED}")

        find_program(GIT_EXECUTABLE git)

        if(GIT_EXECUTABLE)
            message(STATUS "Applying patch using git apply: ${PATCH_FILE}")

            execute_process(
                COMMAND "${GIT_EXECUTABLE}" apply "${PATCH_FILE}"
                WORKING_DIRECTORY "${SOURCE_DIR}"
                RESULT_VARIABLE GIT_APPLY_RESULT
                OUTPUT_VARIABLE GIT_APPLY_OUT
                ERROR_VARIABLE  GIT_APPLY_ERR
            )

            if(GIT_APPLY_RESULT EQUAL 0)                
                file(TOUCH "${PATCH_APPLIED}")
                message(STATUS "Patch applied successfully using git.")
                return()
            else()
                message(WARNING
                    "git apply failed (exit ${GIT_APPLY_RESULT}). "
                    "Falling back to 'patch -p1'.\n"
                    "git output:\n${GIT_APPLY_ERR}"
                )
            endif()
        else()
            message(STATUS "git not found — falling back to patch -p1")
        endif()

        # Find patch tool
        find_program(PATCH_EXECUTABLE patch)

        if(NOT PATCH_EXECUTABLE)
            message(FATAL_ERROR
                "Neither git nor patch utilities are available. "
                "Cannot apply patch: ${PATCH_FILE}"
            )
        endif()

        message(STATUS "Applying patch using patch -p1: ${PATCH_FILE}")

        execute_process(
            COMMAND "${PATCH_EXECUTABLE}" -p1 -i "${PATCH_FILE}"
            WORKING_DIRECTORY "${SOURCE_DIR}"
            RESULT_VARIABLE PATCH_RESULT
            OUTPUT_VARIABLE PATCH_OUT
            ERROR_VARIABLE  PATCH_ERR
        )

        if(PATCH_RESULT EQUAL 0)
            file(TOUCH "${PATCH_APPLIED}")
            message(STATUS "Patch applied successfully using patch.")
        else()
            message(FATAL_ERROR
                "Failed to apply patch using patch -p1: ${PATCH_FILE}\n"
                "Error:\n${PATCH_ERR}"
            )
        endif()
    else()
        get_filename_component(PATCH_NAME "${PATCH_FILE}" NAME)
        message(STATUS "Skipping ${PATCH_NAME}, already applied")
    endif()
endfunction()
