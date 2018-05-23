set(TARGET WebSocket)

add_library(${TARGET} SHARED
        Module.cpp
        URL.cpp
        WebSerializer.cpp
        WebSocketLink.cpp
        )

find_package(ZLIB REQUIRED)

set(PUBLIC_HEADERS
        URL.h
        WebLink.h
        WebRequest.h
        WebResponse.h
        WebSerializer.h
        websocket.h
        WebSocketLink.h
        WebTransfer.h
        WebTransform.h
        Module.h
        )

target_link_libraries(${TARGET}
        PRIVATE
          ZLIB::ZLIB
        PUBLIC #
          ${NAMESPACE}::Core
          ${NAMESPACE}::Cryptalgo
        )

set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        OUTPUT_NAME ${WEBSOCKET_ARTIFACT}
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
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/websocket COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/websocket      # headers
)

HandleStandardLib()

