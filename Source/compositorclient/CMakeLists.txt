set(TARGET compositorclient)

message("Setup ${TARGET} v${VERSION}...")

add_subdirectory(Wayland)

install(
	FILES Client.h 
        DESTINATION include/${NAMESPACE}/compositor
)
