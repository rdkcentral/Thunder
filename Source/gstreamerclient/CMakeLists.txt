set(TARGET gstreamerclient)

option(GSTREAMER_CLIENT_IMPLEMENTATION "Defines which implementation is used." "None")

message("Setup ${TARGET} v${VERSION}...")

if (NOT "${GSTREAMER_CLIENT_IMPLEMENTATION}" STREQUAL "None")

	message("GstreamerClient impl. for ${GSTREAMER_CLIENT_IMPLEMENTATION}")
	find_package(GSTREAMER REQUIRED)
	find_package(GSTREAMER_BASE REQUIRED)

	add_subdirectory (${GSTREAMER_CLIENT_IMPLEMENTATION})
endif()

install(
	FILES gstreamerclient.h
        DESTINATION include/${NAMESPACE}/gstreamerclient/
)
