# Creates a symlink (if used in CMakeLists.txt in configuration phase)
# 
# example: ${CMAKE_PROJECT_DIR}/include/link.h > ${CMAKE_BINARY_DIR}/target.h
#
# include(CreateLink)
# CreateLink(
#    LINK "${CMAKE_PROJECT_DIR}/include/link.h"
#    TARGET "${CMAKE_BINARY_DIR}/target.h"
#)
# 
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
        file(CREATE_LINK ${Argument_TARGET} ${Argument_LINK} SYMBOLIC)
    endif()
endfunction(CreateLink)
