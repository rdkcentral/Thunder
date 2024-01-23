#============================================================================
# Copyright (c) 2017 Liberty Global
#============================================================================

# - Try to find GIO-UNIX
#
# Once done this will define
#  LIBGIOUNIX_FOUND           - System has the component
#  LIBGIOUNIX_INCLUDE_DIRS    - Component include directories
#  LIBGIOUNIX_LIBRARIES       - Libraries needed to use the component

# Use the pkgconfig
find_package(PkgConfig REQUIRED)

# Find the component information
pkg_check_modules(PC_LIBGIOUNIX QUIET gio-unix-2.0)

# <XPREFIX>_FOUND          - set to 1 if module(s) exist
# <XPREFIX>_LIBRARIES      - only the libraries (w/o the '-l')
# <XPREFIX>_LIBRARY_DIRS   - the paths of the libraries (w/o the '-L')
# <XPREFIX>_LDFLAGS        - all required linker flags
# <XPREFIX>_LDFLAGS_OTHER  - all other linker flags
# <XPREFIX>_INCLUDE_DIRS   - the '-I' preprocessor flags (w/o the '-I')
# <XPREFIX>_CFLAGS         - all required cflags
# <XPREFIX>_CFLAGS_OTHER   - the other compiler flags

# <XPREFIX>_VERSION    - version of the module
# <XPREFIX>_PREFIX     - prefix-directory of the module
# <XPREFIX>_INCLUDEDIR - include-dir of the module
# <XPREFIX>_LIBDIR     - lib-dir of the module

message(STATUS "PC_LIBGIOUNIX_FOUND         = ${PC_LIBGIOUNIX_FOUND}")
message(STATUS "PC_LIBGIOUNIX_LIBRARIES     = ${PC_LIBGIOUNIX_LIBRARIES}")
message(STATUS "PC_LIBGIOUNIX_LIBRARY_DIRS  = ${PC_LIBGIOUNIX_LIBRARY_DIRS}")
message(STATUS "PC_LIBGIOUNIX_LDFLAGS       = ${PC_LIBGIOUNIX_LDFLAGS}")
message(STATUS "PC_LIBGIOUNIX_LDFLAGS_OTHER = ${PC_LIBGIOUNIX_LDFLAGS_OTHER}")
message(STATUS "PC_LIBGIOUNIX_INCLUDE_DIRS  = ${PC_LIBGIOUNIX_INCLUDE_DIRS}")
message(STATUS "PC_LIBGIOUNIX_CFLAGS        = ${PC_LIBGIOUNIX_CFLAGS}")
message(STATUS "PC_LIBGIOUNIX_CFLAGS_OTHER  = ${PC_LIBGIOUNIX_CFLAGS_OTHER}")
message(STATUS "PC_LIBGIOUNIX_VERSION       = ${PC_LIBGIOUNIX_VERSION}")
message(STATUS "PC_LIBGIOUNIX_PREFIX        = ${PC_LIBGIOUNIX_PREFIX}")
message(STATUS "PC_LIBGIOUNIX_INCLUDEDIR    = ${PC_LIBGIOUNIX_INCLUDEDIR}")
message(STATUS "PC_LIBGIOUNIX_LIBDIR        = ${PC_LIBGIOUNIX_LIBDIR}")

find_path(LIBGIOUNIX_INCLUDE_DIR
          NAMES gio/gunixfdlist.h
          HINTS ${PC_LIBGIOUNIX_INCLUDEDIR} ${PC_LIBGIOUNIX_INCLUDE_DIRS}
          PATH_SUFFIXES gio-unix-2.0 )

find_library(LIBGIOUNIX_LIBRARY
             NAMES gio-2.0
             HINTS ${PC_LIBGIOUNIX_LIBDIR} ${PC_LIBGIOUNIX_LIBRARY_DIRS} )

message(STATUS "LIBGIOUNIX_INCLUDE_DIR           = ${LIBGIOUNIX_INCLUDE_DIR}")
message(STATUS "LIBGIOUNIX_LIBRARY               = ${LIBGIOUNIX_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set component to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGioUnix DEFAULT_MSG
                                  LIBGIOUNIX_LIBRARY LIBGIOUNIX_INCLUDE_DIR)

mark_as_advanced(LIBGIOUNIX_INCLUDE_DIR LIBGIOUNIX_LIBRARY)

set(LIBGIOUNIX_INCLUDE_DIRS ${LIBGIOUNIX_INCLUDE_DIR})
set(LIBGIOUNIX_LIBRARIES ${LIBGIOUNIX_LIBRARY})

