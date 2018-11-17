# - Try to find libvirtualkeyboard
# Once done this will define
#  LIBBROADCAST_FOUND - System has libvirtualkeyboard
#  LIBBROADCAST_INCLUDE_DIRS - The libvirtualkeyboard include directories
#  LIBBROADCAST_LIBRARIES - The libraries needed to use libvirtualkeyboard
#
# Copyright (C) 2016 Metrological.
#
find_package(PkgConfig)
pkg_check_modules(LIBBROADCAST REQUIRED broadcast)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBBROADCAST DEFAULT_MSG
        LIBBROADCAST_FOUND
        LIBBROADCAST_INCLUDE_DIRS
        LIBBROADCAST_LIBRARIES
        )

mark_as_advanced(LIBBROADCAST_INCLUDE_DIRS LIBBROADCAST_FOUND)
