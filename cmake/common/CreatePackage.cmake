# ----------------------------------------------------------------------------------------
# CreateDepPackage
# ----------------------------------------------------------------------------------------
# Create a DEP package. As seperate call to make target "package" 
# is needed to generate the actual package after building.
#
#    mandatory:
#    COMPONENTS  the cmake components or targets to include in the packags. 
#                each entry will create a seperate package 
#    optional:
#    NAME        Name of the package, defaults to PROJECT_NAME of 
#                the latest cmake project call.
#    VERSION     Version of the package, defaults to PROJECT_VERSION
#                of the latest cmake project call.
#    DESCRIPTION Description of the package
#    MAINTAINER  Package maintainer, defaults to "Metrological"
#    DEPENDS     Other cmake components dependencies if applicable.
#
# ----------------------------------------------------------------------------------------
macro(CreateDepPackage)
    set(optionsArgs )
    set(oneValueArgs NAME MAINTAINER VERSION DESCRIPTION FILENAME)
    set(multiValueArgs COMPONENTS DEPENDS)

    cmake_parse_arguments(ARG "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
    
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to CreateDepPackage(): \"${ARG_UNPARSED_ARGUMENTS}\"")
    endif()

    list(LENGTH ARG_COMPONENTS _NCOMPONENTS)

    if(_NCOMPONENTS GREATER 0)  
        if(ARG_NAME) 
            set(_PACKAGE_NAME ${ARG_NAME})
        else()
            set(_PACKAGE_NAME ${PROJECT_NAME})
        endif()

        if(ARG_MAINTAINER)
            set(_PACKAGE_MAINTAINER ${ARG_MAINTAINER} )
        else()
            set(_PACKAGE_MAINTAINER "Metrological")
        endif()

        if(ARG_VERSION)
            set(_PACKAGE_VERSION ${ARG_VERSION})
        else()
            set(_PACKAGE_VERSION ${PROJECT_VERSION})
        endif() 

        if(ARG_DESCRIPTION)
            set(_PACKAGE_DESCRIPTION ${ARG_DESCRIPTION})
        else()
            set(_PACKAGE_DESCRIPTION "Some epic piece of software")
        endif() 

        message(STATUS "Creating ${CMAKE_SYSTEM_PROCESSOR} package ${_PACKAGE_NAME}-${_PACKAGE_VERSION}")  

        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEB_COMPONENT_INSTALL ON)
        set(CPACK_COMPONENTS_GROUPING IGNORE)

        set(CPACK_DEBIAN_PACKAGE_NAME "${_PACKAGE_NAME}")
        set(CPACK_DEBIAN_PACKAGE_VERSION "${_PACKAGE_VERSION}")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${_PACKAGE_MAINTAINER}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${_PACKAGE_DESCRIPTION}")

        if(ARG_FILENAME)
            set(CPACK_PACKAGE_FILE_NAME "${ARG_FILENAME}")
        endif()

        if(ARG_DEPENDS)
            set(CPACK_DEBIAN_PACKAGE_DEPENDS ${ARG_DEPENDS})
        endif()

        # list of components from which packages will be generated
        set(CPACK_COMPONENTS_ALL ${ARG_COMPONENTS})

        include(CPack)
    else()
        message(FATAL_ERROR "No targets found for CreatePackage")
    endif()
endmacro()