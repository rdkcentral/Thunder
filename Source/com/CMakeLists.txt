set(TARGET COM)

add_library(${TARGET} SHARED
        Administrator.cpp
        Communicator.cpp
        ITracing.cpp
        IStringIterator.cpp
        IUnknown.cpp
        Module.cpp
        )

set(PUBLIC_HEADERS
        Administrator.h
        com.h
        Communicator.h
        IStringIterator.h
        ITracing.h
        IUnknown.h
        Messages.h
        Module.h
        ObjectMessageHandler.h
        Module.h
        )

target_link_libraries(${TARGET}
        PUBLIC
          ${NAMESPACE}::Core
          ${NAMESPACE}::Tracing
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${COM_ARTIFACT}
        )


install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/com COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/com      # headers
)

HandleStandardLib()
