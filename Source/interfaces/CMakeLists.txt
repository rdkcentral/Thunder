set(TargetMarshalling ${NAMESPACE}Marshallings)
set(TargetDefinitions ${NAMESPACE}Definitions)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

file(GLOB PUBLIC_HEADERS I*.h)
file(GLOB JSON_DATA_HEADERS json/JsonData*.h)
file(GLOB PROXY_STUB_SOURCES ProxyStubs*.cpp)
file(GLOB JSON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/json/*API.json)

list(APPEND PUBLIC_HEADERS Module.h)
list(APPEND PUBLIC_HEADERS definitions.h)

ProxyStubGenerator(INPUT ${CMAKE_CURRENT_SOURCE_DIR})
JsonGenerator(CODE INPUT ${JSON_FILE})

add_library(${TargetMarshalling} SHARED ${PROXY_STUB_SOURCES})

add_library(${TargetDefinitions} SHARED
        Definitions.cpp
        json/Enumerations.cpp
        )

target_link_libraries(${TargetMarshalling}
        PRIVATE
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        )

target_link_libraries(${TargetDefinitions}
        PUBLIC
          ${NAMESPACE}Core::${NAMESPACE}Core
          ${NAMESPACE}Tracing::${NAMESPACE}Tracing
        )

if (PROTOCOLS)
    target_link_libraries(${TargetMarshalling}
            PRIVATE
              ${NAMESPACE}Protocols::${NAMESPACE}Protocols
            )
else()
    target_link_libraries(${TargetMarshalling}
            PRIVATE
              ${NAMESPACE}WebSocket::${NAMESPACE}WebSocket
              ${NAMESPACE}COM::${NAMESPACE}COM
            )
endif ()

set_target_properties(${TargetMarshalling} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        )

set_target_properties(${TargetDefinitions} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        VERSION ${VERSION}
        SOVERSION ${VERSION_MAJOR}
        PUBLIC_HEADER "${PUBLIC_HEADERS}" # specify the public headers
        )

target_include_directories(${TargetDefinitions}
        PUBLIC
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../>
          $<INSTALL_INTERFACE:include/${NAMESPACE}>
        )

install(
        TARGETS ${TargetDefinitions} EXPORT ${TargetDefinitions}Targets  # for downstream dependencies
        ARCHIVE DESTINATION lib/ COMPONENT libs      # static lib
        LIBRARY DESTINATION lib/ COMPONENT libs      # shared lib
        RUNTIME DESTINATION bin COMPONENT libs      # binaries
        FRAMEWORK DESTINATION bin/ COMPONENT libs    # for mac
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/interfaces COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}   # default include path
)

install(
        TARGETS ${TargetMarshalling} EXPORT ${TargetMarshalling}Targets  # for downstream dependencies
        LIBRARY DESTINATION lib/${NAMESPACE_LIB}/proxystubs COMPONENT libs      # shared lib
)

install(
        FILES ${JSON_DATA_HEADERS}
        DESTINATION include/${NAMESPACE}/interfaces/json
)

InstallPackageConfig(
        TARGETS ${TargetDefinitions}
        DESCRIPTION "Share lookup tables associated with interface enums."
        )

InstallCMakeConfig(TARGETS ${TargetDefinitions})
