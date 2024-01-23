#============================================================================
# Copyright (c) 2017 Liberty Global
#============================================================================

# - Try to find GIO
#
# Once done this will define
#  LIBGIO_FOUND           - System has the component
#  LIBGIO_INCLUDE_DIRS    - Component include directories
#  LIBGIO_LIBRARIES       - Libraries needed to use the component

# Use the pkgconfig
find_package(PkgConfig REQUIRED)

# Find the component information
pkg_check_modules(PC_LIBGIO QUIET gio-2.0)

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

message(STATUS "PC_LIBGIO_FOUND         = ${PC_LIBGIO_FOUND}")
message(STATUS "PC_LIBGIO_LIBRARIES     = ${PC_LIBGIO_LIBRARIES}")
message(STATUS "PC_LIBGIO_LIBRARY_DIRS  = ${PC_LIBGIO_LIBRARY_DIRS}")
message(STATUS "PC_LIBGIO_LDFLAGS       = ${PC_LIBGIO_LDFLAGS}")
message(STATUS "PC_LIBGIO_LDFLAGS_OTHER = ${PC_LIBGIO_LDFLAGS_OTHER}")
message(STATUS "PC_LIBGIO_INCLUDE_DIRS  = ${PC_LIBGIO_INCLUDE_DIRS}")
message(STATUS "PC_LIBGIO_CFLAGS        = ${PC_LIBGIO_CFLAGS}")
message(STATUS "PC_LIBGIO_CFLAGS_OTHER  = ${PC_LIBGIO_CFLAGS_OTHER}")
message(STATUS "PC_LIBGIO_VERSION       = ${PC_LIBGIO_VERSION}")
message(STATUS "PC_LIBGIO_PREFIX        = ${PC_LIBGIO_PREFIX}")
message(STATUS "PC_LIBGIO_INCLUDEDIR    = ${PC_LIBGIO_INCLUDEDIR}")
message(STATUS "PC_LIBGIO_LIBDIR        = ${PC_LIBGIO_LIBDIR}")

find_path(LIBGIO_INCLUDE_DIR
          NAMES gio/gio.h
          HINTS ${PC_LIBGIO_INCLUDEDIR} ${PC_LIBGIO_INCLUDE_DIRS}
          PATH_SUFFIXES glib-2.0 )

find_library(LIBGIO_LIBRARY
             NAMES gio-2.0
             HINTS ${PC_LIBGIO_LIBDIR} ${PC_LIBGIO_LIBRARY_DIRS} )

message(STATUS "LIBGIO_INCLUDE_DIR           = ${LIBGIO_INCLUDE_DIR}")
message(STATUS "LIBGIO_LIBRARY               = ${LIBGIO_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set component to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGio DEFAULT_MSG
                                  LIBGIO_LIBRARY LIBGIO_INCLUDE_DIR)

mark_as_advanced(LIBGIO_INCLUDE_DIR LIBGIO_LIBRARY)

set(LIBGIO_INCLUDE_DIRS ${LIBGIO_INCLUDE_DIR})
set(LIBGIO_LIBRARIES ${LIBGIO_LIBRARY})

