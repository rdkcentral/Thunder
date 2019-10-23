set(TARGET gstreamerclient)

option(GSTREAMER_CLIENT_IMPLEMENTATION "Defines which implementation is used." "None")

message("Setup ${TARGET} v${VERSION}...")

if (NOT "${GSTREAMER_CLIENT_IMPLEMENTATION}" STREQUAL "None")
	add_subdirectory (${GSTREAMER_CLIENT_IMPLEMENTATION})
endif()

install(
	FILES GstreamerClient.h
        DESTINATION include/${NAMESPACE}/gstreamerclient/
)
