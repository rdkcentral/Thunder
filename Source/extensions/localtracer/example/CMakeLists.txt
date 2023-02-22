add_executable(local_trace_test
    main.cpp
)

target_link_libraries(local_trace_test PRIVATE
    CompileSettingsDebug::CompileSettingsDebug
    ${NAMESPACE}Core::${NAMESPACE}Core
    ${NAMESPACE}LocalTracer::${NAMESPACE}LocalTracer)

if(EXAMPLE_LOCALTRACER)
    install(TARGETS local_trace_test DESTINATION bin)
endif()
