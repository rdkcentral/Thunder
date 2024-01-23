#============================================================================
# Copyright (c) 2017 Liberty Global
#============================================================================

# - Try to find Gobject
#
# Once done this will define
#  LIBGOBJECT_FOUND           - System has the component
#  LIBGOBJECT_INCLUDE_DIRS    - Component include directories
#  LIBGOBJECT_LIBRARIES       - Libraries needed to use the component

# Use the pkgconfig
find_package(PkgConfig REQUIRED)

# Find the component information
pkg_check_modules(PC_LIBGOBJECT QUIET gobject-2.0)

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

message(STATUS "PC_LIBGOBJECT_FOUND         = ${PC_LIBGOBJECT_FOUND}")
message(STATUS "PC_LIBGOBJECT_LIBRARIES     = ${PC_LIBGOBJECT_LIBRARIES}")
message(STATUS "PC_LIBGOBJECT_LIBRARY_DIRS  = ${PC_LIBGOBJECT_LIBRARY_DIRS}")
message(STATUS "PC_LIBGOBJECT_LDFLAGS       = ${PC_LIBGOBJECT_LDFLAGS}")
message(STATUS "PC_LIBGOBJECT_LDFLAGS_OTHER = ${PC_LIBGOBJECT_LDFLAGS_OTHER}")
message(STATUS "PC_LIBGOBJECT_INCLUDE_DIRS  = ${PC_LIBGOBJECT_INCLUDE_DIRS}")
message(STATUS "PC_LIBGOBJECT_CFLAGS        = ${PC_LIBGOBJECT_CFLAGS}")
message(STATUS "PC_LIBGOBJECT_CFLAGS_OTHER  = ${PC_LIBGOBJECT_CFLAGS_OTHER}")
message(STATUS "PC_LIBGOBJECT_VERSION       = ${PC_LIBGOBJECT_VERSION}")
message(STATUS "PC_LIBGOBJECT_PREFIX        = ${PC_LIBGOBJECT_PREFIX}")
message(STATUS "PC_LIBGOBJECT_INCLUDEDIR    = ${PC_LIBGOBJECT_INCLUDEDIR}")
message(STATUS "PC_LIBGOBJECT_LIBDIR        = ${PC_LIBGOBJECT_LIBDIR}")

find_path(LIBGOBJECT_INCLUDE_DIR
          NAMES gobject/gobject.h
          HINTS ${PC_LIBGOBJECT_INCLUDEDIR} ${PC_LIBGOBJECT_INCLUDE_DIRS}
)

find_library(LIBGOBJECT_LIBRARY
             NAMES gobject-2.0
             HINTS ${PC_LIBGOBJECT_LIBDIR} ${PC_LIBGOBJECT_LIBRARY_DIRS}
)

message(STATUS "LIBGOBJECT_INCLUDE_DIR           = ${LIBGOBJECT_INCLUDE_DIR}")
message(STATUS "LIBGOBJECT_LIBRARY               = ${LIBGOBJECT_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set component to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGobject DEFAULT_MSG
                                  LIBGOBJECT_LIBRARY LIBGOBJECT_INCLUDE_DIR)

mark_as_advanced(LIBGOBJECT_INCLUDE_DIR LIBGOBJECT_LIBRARY)

set(LIBGOBJECT_INCLUDE_DIRS ${LIBGOBJECT_INCLUDE_DIR})
set(LIBGOBJECT_LIBRARIES ${LIBGOBJECT_LIBRARY})
