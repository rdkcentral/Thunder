set(TARGET VirtualInput)

option(VIRTUALINPUT_TOOLS "Build VirtualInput tools" OFF)

add_library(${TARGET} SHARED
        Module.cpp
        VirtualKeyboard.cpp
        )

set(PUBLIC_HEADERS
        virtualinput.h
        VirtualKeyboard.h
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
        OUTPUT_NAME ${VIRTUALINPUT_ARTIFACT}
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
        PUBLIC_HEADER DESTINATION include/${NAMESPACE}/virtualinput COMPONENT devel   # headers for mac (note the different component -> different package)
        INCLUDES DESTINATION include/${NAMESPACE}/virtualinput      # headers
)

HandleStandardLib()

if (VIRTUALINPUT_TOOLS)
    add_subdirectory(tools)
endif()


configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/virtualkeyboard.pc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/virtualkeyboard.pc"
        @ONLY)

install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/virtualkeyboard.pc
        DESTINATION lib/pkgconfig
)
