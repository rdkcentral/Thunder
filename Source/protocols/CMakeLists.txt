set(TARGET Protocols)

find_package(ZLIB REQUIRED)

# Take care the sources are listed !!! TWICE !!! in this setup. raised a JIRA ticket to solve this for now, also scroll down and list the 
# new source file again. This is in case CMAKE forgets :-)

set(SOURCES_COM
        com/Administrator.h
        com/com.h
        com/Communicator.h
        com/ITracing.h
        com/IUnknown.h
        com/Messages.h
        com/ObjectMessageHandler.h
        com/Administrator.cpp
        com/Communicator.cpp
        com/ITracing.cpp
        com/IStringIterator.cpp
        com/IUnknown.cpp
        com/IStringIterator.h
        )

set(SOURCES_WEBSOCKETS
        websocket/URL.cpp
        websocket/WebSerializer.cpp
        websocket/WebSocketLink.cpp
        websocket/URL.h
        websocket/WebLink.h
        websocket/WebRequest.h
        websocket/WebResponse.h
        websocket/WebSerializer.h
        websocket/websocket.h
        websocket/WebSocketLink.h
        websocket/WebTransfer.h
        websocket/WebTransform.h
        )

foreach(file IN LISTS SOURCES_COM SOURCES_WEBSOCKETS)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../${file}" ${CMAKE_CURRENT_SOURCE_DIR} COPYONLY)
endforeach()

set(COM_INCLUDES
        Administrator.h
        com.h
        Communicator.h
        ITracing.h
        IUnknown.h
        Messages.h
        ObjectMessageHandler.h
        IStringIterator.h
        )

set(PROTOCOLS_INCLUDES
        URL.h
        WebLink.h
        WebRequest.h
        WebResponse.h
        WebSerializer.h
        websocket.h
        WebSocketLink.h
        WebTransfer.h
        WebTransform.h
        )

set(PUBLIC_HEADERS
        ${COM_INCLUDES}
        ${PROTOCOLS_INCLUDES}
        Module.h
        )

# Construct a library object
add_library(${TARGET} SHARED
        Module.cpp
        URL.cpp
        ITracing.cpp
        IUnknown.cpp
        WebSerializer.cpp
        WebSocketLink.cpp
        Administrator.cpp
        Communicator.cpp
        IStringIterator.cpp
        )

target_link_libraries(${TARGET}
        PRIVATE
          ZLIB::ZLIB
          ${NAMESPACE}::Core
        PUBLIC #FIXME: Added PUBLIC to fix linker warning that dependecies were not found
          ${NAMESPACE}::Tracing
          ${NAMESPACE}::Cryptalgo
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${PROTOCOLS_ARTIFACT}
        )


install(
        TARGETS ${TARGET}  EXPORT ${TARGET}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib COMPONENT libs      # static lib
        LIBRARY DESTINATION lib COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/protocols COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/protocols      # headers
)

HandleStandardLib()

#FIXME: This is messy, but it's the consequence of combining more independent libraries in one.
install(
        FILES ${COM_INCLUDES} ../com/Module.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/com
)

install(
        FILES ${PROTOCOLS_INCLUDES} ../websocket/Module.h
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${NAMESPACE}/websocket
)
