find_package(Threads)

add_executable(WPELibraryTestLoader main.cpp)

target_link_libraries(WPELibraryTestLoader
        PRIVATE
          Threads::Threads
          ${CMAKE_DL_LIBS}
          ${NAMESPACE}Core::${NAMESPACE}Core
        )

set_target_properties(WPELibraryTestLoader PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )

install(TARGETS WPELibraryTestLoader DESTINATION bin)

