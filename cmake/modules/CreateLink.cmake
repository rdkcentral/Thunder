function(CreateLink)
    set(oneValueArgs TARGET LINK)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    message(STATUS "Creating symlink ${Argument_LINK} -> ${Argument_TARGET}")

    get_filename_component(LINK_DIR ${Argument_LINK} DIRECTORY)
    file(MAKE_DIRECTORY "${LINK_DIR}")

    if(${CMAKE_VERSION} VERSION_LESS "3.14.0") 
        message(WARNING "Please consider to switch to CMake 3.14.0 or up to create symlinks")
        INSTALL(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${Argument_TARGET} ${Argument_LINK})")
    else()
        file(CREATE_LINK ${Argument_TARGET} ${Argument_LINK})
    endif()
endfunction(CreateLink)
