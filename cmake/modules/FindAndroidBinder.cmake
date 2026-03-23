# FindAndroidBinder.cmake
#
# Locates the Android Binder kernel interface headers and (optionally) a
# user-space libbinder_ndk or similar library provided by the host.
#
# On a stock Linux developer machine the only requirement is that
#   <linux/android/binder.h>
# is present in the kernel headers (provided by linux-headers-* on Debian/Ubuntu
# or kernel-headers on Fedora).
#
# If a full libbinder_ndk is available (Android sysroot), this module also
# tries to locate libbinder_ndk.so.
#
# Result variables:
#   AndroidBinder_FOUND         - TRUE if the kernel header is found
#   AndroidBinder_INCLUDE_DIRS  - include dirs (may be empty if the header is
#                                 already in the default compiler search path)
#   AndroidBinder_LIBRARIES     - link libraries (empty if header-only)
#
# Imported targets:
#   AndroidBinder::AndroidBinder
#
# Cache variables:
#   ANDROID_BINDER_INCLUDE_DIR  - override directory for linux/android/binder.h

include(FindPackageHandleStandardArgs)

# ---- locate kernel header ----

find_path(ANDROID_BINDER_INCLUDE_DIR
    NAMES linux/android/binder.h
    PATHS
        /usr/include
        /usr/local/include
        ${CMAKE_SYSROOT}/usr/include
        ${CMAKE_FIND_ROOT_PATH}/usr/include
    DOC "Directory containing linux/android/binder.h"
)

# ---- locate libbinder_ndk (optional, Android sysroot only) ----
find_library(ANDROID_BINDER_LIBRARY
    NAMES binder_ndk
    DOC "Android libbinder_ndk shared library (optional)"
)

# ---- standard result variables ----
find_package_handle_standard_args(AndroidBinder
    REQUIRED_VARS ANDROID_BINDER_INCLUDE_DIR
    FAIL_MESSAGE "Could not find linux/android/binder.h – install linux-headers for binder support"
)

if(AndroidBinder_FOUND)
    set(AndroidBinder_INCLUDE_DIRS "${ANDROID_BINDER_INCLUDE_DIR}")

    if(ANDROID_BINDER_LIBRARY)
        set(AndroidBinder_LIBRARIES "${ANDROID_BINDER_LIBRARY}")
    else()
        set(AndroidBinder_LIBRARIES "")
    endif()

    if(NOT TARGET AndroidBinder::AndroidBinder)
        add_library(AndroidBinder::AndroidBinder INTERFACE IMPORTED)
        set_target_properties(AndroidBinder::AndroidBinder PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${AndroidBinder_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES      "${AndroidBinder_LIBRARIES}"
        )
    endif()
endif()

mark_as_advanced(ANDROID_BINDER_INCLUDE_DIR ANDROID_BINDER_LIBRARY)
