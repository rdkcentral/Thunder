set(TARGET compositorclient)

option(PLUGIN_COMPOSITOR_IMPLEMENTATION "Defines which implementation is used." "None")


message("Setup ${TARGET} v${VERSION}...")

add_subdirectory (${PLUGIN_COMPOSITOR_IMPLEMENTATION})

install(
	FILES Client.h
        DESTINATION include/${NAMESPACE}/compositorclient
)
