# Creates a symlink in the installalation phase
# 
# example: <STAGING_DIR>/usr/include/link.h > target.h
# include(InstallLink)
# InstallLink(
#    LINK "${CMAKE_SYSROOT}${CMAKE_INSTALL_PREFIX}/include/link.h"
#    TARGET "target.h"
#)
# 
macro(InstallLink)
    set(oneValueArgs TARGET LINK)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(LINK "${Argument_LINK}")
    set(TARGET "${Argument_TARGET}")

    configure_file("install_symlink.cmake.in" "install_symlink.cmake"
    @ONLY)

    install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_symlink.cmake")
endmacro(InstallLink)
