add_executable(local_trace_test
    main.cpp
)

target_compile_options(local_trace_test PRIVATE -Wno-psabi)

target_link_libraries(local_trace_test PRIVATE
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer)

if(EXAMPLE_LOCALTRACER)
    install(TARGETS local_trace_test DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${NAMESPACE}_Test)
endif()
