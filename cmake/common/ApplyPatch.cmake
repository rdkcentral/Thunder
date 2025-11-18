function(ApplyPatch SOURCE_DIR PATCH_FILE)
    # Find git
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

    if(NOT PATCH_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Failed to apply patch using patch -p1: ${PATCH_FILE}\n"
            "Error:\n${PATCH_ERR}"
        )
    endif()

    message(STATUS "Patch applied successfully using patch.")
endfunction()
