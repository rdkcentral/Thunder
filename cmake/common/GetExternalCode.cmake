function(GetExternalCode)
    set(optionsArgs FORCE)
    set(oneValueArgs GIT_REPOSITORY GIT_TAG SOURCE_DIR)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to GetExternalCode(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    find_package(Git REQUIRED)

    set(GIT_REPOSITORY "not set")
    set(SOURCE_DIR "git")

    if(Argument_GIT_REPOSITORY)
        set(GIT_REPOSITORY ${Argument_GIT_REPOSITORY})
    endif()

    if(Argument_SOURCE_DIR)
        set(SOURCE_DIR ${Argument_SOURCE_DIR})
    endif()

    if("${SOURCE_DIR}" MATCHES "^/")
        set(REPO_LOCATION ${SOURCE_DIR}) # assume that we specified a absolute path
    else()
        set(REPO_LOCATION ${CMAKE_CURRENT_LIST_DIR}/${SOURCE_DIR})
    endif()

    if(EXISTS ${REPO_LOCATION} AND Argument_FORCE)
        message(STATUS "Removing ${REPO_LOCATION}")
        file(REMOVE_RECURSE "${REPO_LOCATION}") 
    endif()
    
    if(GIT_FOUND)
        if(EXISTS ${REPO_LOCATION}/.git)
            message(STATUS "Git repo detected in ${REPO_LOCATION}, skipping clone")
        else()
            message(STATUS "Cloning ${GIT_REPOSITORY} in ${REPO_LOCATION}")
        
            execute_process(COMMAND ${GIT_EXECUTABLE} clone --depth 1 "${GIT_REPOSITORY}" "${REPO_LOCATION}"
                            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                            RESULT_VARIABLE GIT_CLONE_RESULT)
            if(NOT GIT_CLONE_RESULT EQUAL "0")
                message(FATAL_ERROR "Git clone failed!")
            endif()
        endif()

        if(Argument_GIT_TAG)
            if(EXISTS ${REPO_LOCATION})
                message(STATUS "Checkout ${Argument_GIT_TAG}")
                execute_process(COMMAND ${GIT_EXECUTABLE} checkout ${Argument_GIT_TAG}
                                WORKING_DIRECTORY ${REPO_LOCATION}
                                RESULT_VARIABLE GIT_CLONE_RESULT)
                if(NOT GIT_CLONE_RESULT EQUAL "0")
                    message(FATAL_ERROR "Git checkout of ${Argument_GIT_TAG} failed!")
                endif()
            else()
                message(FATAL_ERROR "Repo ${REPO_LOCATION} not found!")
            endif()
        endif()
    endif()
endfunction()