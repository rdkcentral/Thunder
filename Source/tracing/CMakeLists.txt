set(TARGET Tracing)

add_library(${TARGET} SHARED
        Module.cpp
        TraceCategories.cpp
        TraceMedia.cpp
        TraceUnit.cpp
        )

set(PUBLIC_HEADERS
        ITraceControl.h
        ITraceMedia.h
        TraceCategories.h
        TraceControl.h
        TraceMedia.h
        TraceUnit.h
        tracing.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ${NAMESPACE}::Core
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${TRACING_ARTIFACT}
        )

target_include_directories( ${TARGET}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/tracing COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/tracing      # headers
)

HandleStandardLib()
