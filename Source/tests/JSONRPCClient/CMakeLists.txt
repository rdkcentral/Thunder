find_package(WPEFrameworkJSONRPC REQUIRED)

add_executable(CompositorTest Test.cpp)

set_target_properties(CompositorTest PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        )     

target_link_libraries(JSONRPCTest
    PRIVATE
        WPE
        )

target_include_directories(CompositorTest
    PRIVATE
      ${EGL_INCLUDE_DIRS}
      ${GLESV2_INCLUDE_DIRS})

target_compile_definitions(CompositorTest
    PRIVATE
        ${EGL_DEFINITIONS}
        ${GLESV2_DEFINITIONS})

install(TARGETS CompositorTest DESTINATION bin)
