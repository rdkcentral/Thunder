include(ExternalProject)

function(GetGitRepo url tag target dir)

    # get filename from download URL
    get_filename_component(package_raw "${url}" NAME)
    # replace special characters from filename
    string(REGEX REPLACE "[!@#$%^&*?]" "_" package "${package_raw}")


    ExternalProject_Add(${package}
            GIT_REPOSITORY ${url}
            GIT_TAG ${tag}
            PREFIX ${dir}/${package}
            GIT_SHALLOW 1
            GIT_PROGRESS 1
            UPDATE_COMMAND ""
            PATCH_COMMAND ""
            TEST_COMMAND ""
            BUILD_COMMAND ""
            STEP_TARGETS install build
            EXCLUDE_FROM_ALL TRUE
            )

    set(${target} "${package}" PARENT_SCOPE)
endfunction()