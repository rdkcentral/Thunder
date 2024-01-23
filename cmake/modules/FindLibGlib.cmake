#============================================================================
# Copyright (c) 2017 Liberty Global
#============================================================================

# - Try to find Glib
#
# Once done this will define
#  LIBGLIB_FOUND           - System has the component
#  LIBGLIB_INCLUDE_DIRS    - Component include directories
#  LIBGLIB_LIBRARIES       - Libraries needed to use the component

# Use the pkgconfig
find_package(PkgConfig REQUIRED)

# Find the component information
pkg_check_modules(PC_LIBGLIB QUIET glib-2.0)

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

message(STATUS "PC_LIBGLIB_FOUND         = ${PC_LIBGLIB_FOUND}")
message(STATUS "PC_LIBGLIB_LIBRARIES     = ${PC_LIBGLIB_LIBRARIES}")
message(STATUS "PC_LIBGLIB_LIBRARY_DIRS  = ${PC_LIBGLIB_LIBRARY_DIRS}")
message(STATUS "PC_LIBGLIB_LDFLAGS       = ${PC_LIBGLIB_LDFLAGS}")
message(STATUS "PC_LIBGLIB_LDFLAGS_OTHER = ${PC_LIBGLIB_LDFLAGS_OTHER}")
message(STATUS "PC_LIBGLIB_INCLUDE_DIRS  = ${PC_LIBGLIB_INCLUDE_DIRS}")
message(STATUS "PC_LIBGLIB_CFLAGS        = ${PC_LIBGLIB_CFLAGS}")
message(STATUS "PC_LIBGLIB_CFLAGS_OTHER  = ${PC_LIBGLIB_CFLAGS_OTHER}")
message(STATUS "PC_LIBGLIB_VERSION       = ${PC_LIBGLIB_VERSION}")
message(STATUS "PC_LIBGLIB_PREFIX        = ${PC_LIBGLIB_PREFIX}")
message(STATUS "PC_LIBGLIB_INCLUDEDIR    = ${PC_LIBGLIB_INCLUDEDIR}")
message(STATUS "PC_LIBGLIB_LIBDIR        = ${PC_LIBGLIB_LIBDIR}")

find_path(LIBGLIB_INCLUDE_DIR
          NAMES glib.h
          HINTS ${PC_LIBGLIB_INCLUDEDIR} ${PC_LIBGLIB_INCLUDE_DIRS}
          PATH_SUFFIXES glib-2.0 )

find_path(LIBGLIB_CONFIG_INCLUDE_DIR
          NAMES glibconfig.h
          HINTS ${PC_LIBGLIB_INCLUDEDIR} ${PC_LIBGLIB_INCLUDE_DIRS}
          PATH_SUFFIXES lib/glib-2.0/include ../lib/glib-2.0/include )

find_library(LIBGLIB_LIBRARY
             NAMES glib-2.0
             HINTS ${PC_LIBGLIB_LIBDIR} ${PC_LIBGLIB_LIBRARY_DIRS} )

message(STATUS "LIBGLIB_INCLUDE_DIR           = ${LIBGLIB_INCLUDE_DIR}")
message(STATUS "LIBGLIB_CONFIG_INCLUDE_DIR    = ${LIBGLIB_CONFIG_INCLUDE_DIR}")
message(STATUS "LIBGLIB_LIBRARY               = ${LIBGLIB_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set component to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGlib DEFAULT_MSG
                                  LIBGLIB_LIBRARY LIBGLIB_INCLUDE_DIR LIBGLIB_CONFIG_INCLUDE_DIR)

mark_as_advanced(LIBGLIB_INCLUDE_DIR LIBGLIB_CONFIG_INCLUDE_DIR LIBGLIB_LIBRARY)

set(LIBGLIB_INCLUDE_DIRS ${LIBGLIB_INCLUDE_DIR} ${LIBGLIB_CONFIG_INCLUDE_DIR})
set(LIBGLIB_LIBRARIES ${LIBGLIB_LIBRARY})
