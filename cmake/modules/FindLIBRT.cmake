## clock-monotonic, just see if we need to link with rt
set(CMAKE_REQUIRED_LIBRARIES_SAVE ${CMAKE_REQUIRED_LIBRARIES})
set(CMAKE_REQUIRED_LIBRARIES rt)
CHECK_SYMBOL_EXISTS(_POSIX_TIMERS "unistd.h;time.h" POSIX_TIMERS_FOUND)
set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES_SAVE})

if(POSIX_TIMERS_FOUND)
    find_library(LIBRT_LIBRARY NAMES rt)

    if(NOT TARGET LIBRT::LIBRT)
        add_library(LIBRT::LIBRT UNKNOWN IMPORTED)
        if(EXISTS "${LIBRT_LIBRARY}")
            set_target_properties(LIBRT::LIBRT PROPERTIES
                    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                    IMPORTED_LOCATION "${LIBRT_LIBRARY}")
        endif()
    endif()

    mark_as_advanced(LIBRT_LIBRARY)
endif()
