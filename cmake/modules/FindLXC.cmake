find_package(PkgConfig)
pkg_check_modules(PC_LXC lxc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PC_LXC DEFAULT_MSG PC_LXC_FOUND)

mark_as_advanced(PC_LXC_INCLUDE_DIRS PC_LXC_LIBRARIES PC_LXC_LIBRARY_DIRS)

if(${PC_LXC_FOUND})

    find_library(LXC_LIBRARY lxc
        HINTS ${PC_LXC_LIBDIR} ${PC_LXC_LIBRARY_DIRS}
    )

    set(LXC_LIBRARIES ${PC_LXC_LIBRARIES})
    set(LXC_INCLUDES ${PC_LXC_INCLUDE_DIRS})
    set(LXC_FOUND ${PC_LXC_FOUND})

    if(NOT TARGET LXC::LXC)
        add_library(LXC::LXC UNKNOWN IMPORTED)

        set_target_properties(LXC::LXC
                PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${LXC_LIBRARY}"
                INTERFACE_COMPILE_DEFINITIONS "LXC"
                INTERFACE_COMPILE_OPTIONS "${PC_LXC_CFLAGS_OTHER}"
                INTERFACE_INCLUDE_DIRECTORIES "${PC_LXC_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${PC_LXC_LIBRARIES}"
                )
    endif()
endif()