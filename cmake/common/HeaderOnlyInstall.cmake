function(HeaderOnlyInstallCMakeConfig)
    set(optionsArgs NO_SKIP_INTERFACE_LIBRARIES, TREAT_AS_NORMAL)
    set(oneValueArgs LOCATION TEMPLATE TARGET)
    set(multiValueArgs EXTRA_DEPENDENCIES)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to HeaderOnlyInstallCMakeConfig(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    set(_install_path "lib/cmake") # default path

    set(TARGET ${Argument_TARGET})
    set(NAME ${Argument_NAME})

    if(Agument_LOCATION)
        set(_install_path "${Agument_LOCATION}" FORCE)
    endif()

    if("${Argument_TEMPLATE}" STREQUAL "")
        find_file( _config_template
            NAMES "defaultConfig.cmake.in"
            PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates" "${CMAKE_INSTALL_PREFIX}/lib/cmake/${NAMESPACE}/templates"
            NO_DEFAULT_PATH
            NO_CMAKE_ENVIRONMENT_PATH
            NO_CMAKE_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_FIND_ROOT_PATH)

        find_file(_config_template
            NAMES "defaultConfig.cmake.in"
            PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates" "${CMAKE_INSTALL_PREFIX}/lib/cmake/${NAMESPACE}/templates" )

        if(NOT EXISTS "${_config_template}")
            message(SEND_ERROR "Config file generation failed, template '${_config_template}' not found")
            return()
        endif()
    elseif(EXISTS "${Argument_TEMPLATE}")
        set(_config_template  ${Argument_TEMPLATE})
    else()
        message(SEND_ERROR "Cmake config file generation, template '${_config_template}' not found")
        return()
    endif()

    # add manual dependecies to look for...
    # make sure its a valid list with valid names for find_package
    foreach(_extra_dep ${Argument_EXTRA_DEPENDENCIES})
        list(APPEND dependencies  ${_extra_dep})
    endforeach()

    if(NOT TARGET ${TARGET})
        message(SEND_ERROR "There is no target named '${TARGET}'")
        break()
    endif()

    get_target_property(_type ${TARGET} TYPE)

    if(NOT "${_type}" STREQUAL "INTERFACE_LIBRARY")
        get_target_property(_version ${TARGET} VERSION)
        get_target_property(_name ${TARGET} OUTPUT_NAME)
    endif()

    if(NOT "${_type}" STREQUAL "INTERFACE_LIBRARY" OR Argument_TREAT_AS_NORMAL)
        get_target_property(_dependencies ${TARGET} INTERFACE_LINK_LIBRARIES)
    endif()

    if(NOT _name)
        get_target_property(_name ${TARGET} NAME)
    endif()

    if (NOT _name)
        message(FATAL_ERROR "right... ${TARGET} should have a name right, time for coffee?!")
    endif()

    if(NOT _version)
        set(_version 0.0.0)
    endif()

    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake
        VERSION ${_version}
        COMPATIBILITY SameMajorVersion)

    message(STATUS "${TARGET} added support for cmake consumers via '${_name}Config.cmake'")

    if(NOT "${_type}" STREQUAL "INTERFACE_LIBRARY" OR Argument_TREAT_AS_NORMAL)
        # The alias is used by local targets project
        add_library(${_name}::${_name} ALIAS ${TARGET})

        if(_dependencies)
            foreach(_dependency ${_dependencies})
                if("${_dependency}" MATCHES "LINK_ONLY")
                    string(REGEX REPLACE "\\$<LINK_ONLY:" "" __fix ${_dependency})
                    string(REGEX REPLACE ">$" "" _dependency ${__fix})
                endif()

                if(TARGET ${_dependency})
                    get_target_property(_type ${_dependency} TYPE)

                    if(NOT "${_type}" STREQUAL "INTERFACE_LIBRARY" OR Argument_NO_SKIP_INTERFACE_LIBRARIES)
                        set(_type_is_ok TRUE)
                    else()
                        set(_type_is_ok FALSE)
                    endif()

                    get_target_property(_is_imported ${_dependency} IMPORTED)

                    if(_type_is_ok)
                        if(_is_imported)
                            get_target_property(_configurations ${_dependency} IMPORTED_CONFIGURATIONS)

                            if (_configurations)
                                list(LENGTH _configurations _configurations_count)

                                list(GET _configurations 0 _config)
                                set (config _${_config})

                                if (_configurations_count GREATER 1)
                                    message(AUTHOR_WARNING "Multiple configs not yet supported, got ${_configurations} and picked the first one")
                                endif()
                            endif()

                            get_target_property(_dep_loc ${_dependency} IMPORTED_LOCATION${config})

                            _get_default_link_name(${_dep_loc} _dep_name _dep_dir)
                        else()
                            get_target_property(_dep_name ${_dependency} OUTPUT_NAME)

                            if(NOT _dep_name)
                                get_target_property( _dep_name ${_dependency} NAME)
                            endif()
                        endif(_is_imported)

                        if(_dep_name)
                            list(APPEND dependencies  ${_dep_name})
                        endif()
                    endif()
                endif()
            endforeach()
        endif()
    endif()

    # message(FATAL_ERROR "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake")

    configure_file( "${_config_template}"
            "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake"
            @ONLY)

    install(
        EXPORT "${TARGET}Targets"
        FILE "${_name}Targets.cmake"
        NAMESPACE  "${_name}::"
        DESTINATION "${_install_path}/${_name}")

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake"
        DESTINATION "${_install_path}/${_name}")

endfunction(HeaderOnlyInstallCMakeConfig)

function(HeaderOnlyInstallPackageConfig)
    set(optionsArgs NO_DEFAULT_LIB_DIR_FILTER NO_COMPILE_OPTIONS NO_INCLUDE_DIRS NO_COMPILE_DEFINES)
    set(oneValueArgs TEMPLATE DESCRIPTION LOCATION OUTPUT_NAME NAME VERSION TARGET)
    set(multiValueArgs )

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to HeaderOnlyInstallPackageConfig(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    if("${Argument_TEMPLATE}" STREQUAL "")
        find_file( _pc_template
                    NAMES "default.pc.in"
                    PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates" "${CMAKE_INSTALL_PREFIX}/lib/cmake/${NAMESPACE}/templates"
                    NO_DEFAULT_PATH
                    NO_CMAKE_ENVIRONMENT_PATH
                    NO_CMAKE_PATH
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_FIND_ROOT_PATH)

        find_file(_pc_template
                    NAMES "default.pc.in"
                    PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates" "${CMAKE_INSTALL_PREFIX}/lib/cmake/${NAMESPACE}/templates")

        if(NOT EXISTS "${_pc_template}")
            message(SEND_ERROR "PC file generation failed, template '${_pc_template}' not found")
            return()
        endif()
    elseif(EXISTS "${Argument_TEMPLATE}")
        set(_pc_template  ${Argument_TEMPLATE})
    else()
        message(SEND_ERROR "PC file generation, template '${_template}' not found")
        return()
    endif()

    set(TARGET ${Argument_TARGET})
    set(NAME ${Argument_NAME})

    if (${Argument_LOCATION})
        set(VERSION ${Argument_VERSION})
    endif()

    set(DESCRIPTION "${Argument_DESCRIPTION}")

    if (${Argument_LOCATION})
        string(REPLACE "${CMAKE_SYSROOT}" "" TARGET_LIBRARY_DIR ${Argument_LOCATION})
        string(REPLACE "${CMAKE_INSTALL_PREFIX}" "" TARGET_LIBRARY_DIR ${TARGET_LIBRARY_DIR})
        string(REGEX REPLACE "^(/+)" "" TARGET_LIBRARY_DIR ${TARGET_LIBRARY_DIR})
    else()
        set(TARGET_LIBRARY_DIR "lib")
    endif()


    if(NOT TARGET ${TARGET})
        message(SEND_ERROR "There is no target named '${TARGET}'")
        break()
    endif()

    if (NOT NAME)
        message(FATAL_ERROR "right... ${TARGET} should have a name right, time for coffee?!")
    endif()

    if(NOT VERSION)
        set(VERSION 0.0.0)
    endif()

    # Default path on UNIX, if you want Windows or Apple support add the path here. ;-)
    set(_install_path "lib/pkgconfig")

    if (${Argument_OUTPUT_NAME})
        set(_pc_filename  ${Argument_OUTPUT_NAME})
    else()
        set(_pc_filename  ${NAME}.pc)
    endif()

    if(NOT Argument_NO_COMPILE_OPTIONS)
        get_if_compile_options(compile_options ${TARGET})
        foreach(compile_option ${compile_options})
            set (COMPILE_OPTIONS "${COMPILE_OPTIONS} ${compile_option}")
        endforeach()
    endif()

    if(NOT Argument_NO_COMPILE_DEFINES)
        get_if_compile_defines(defines ${TARGET})
        foreach(define ${defines})
            set (COMPILE_DEFINES "${COMPILE_DEFINES} -D${define}")
        endforeach()
    endif()

    if(NOT Argument_NO_INCLUDE_DIRS)
        get_if_include_dirs(includes ${TARGET})
        list(LENGTH includes _includes_count)

        if (_includes_count GREATER 0)
            # Assuming the first in the list belongs to TARGET
            list(GET includes 0 TARGET_INCLUDE_DIR)
            list(REMOVE_AT includes 0)
            string(REPLACE "${CMAKE_INSTALL_PREFIX}/" "" TARGET_INCLUDE_DIR ${TARGET_INCLUDE_DIR})
        else()
            set(TARGET_INCLUDE_DIR "include/")
        endif()
    else()
        set(TARGET_INCLUDE_DIR "include/")
    endif()

    foreach(include ${includes})
        set (INCLUDE_DIRS "${INCLUDE_DIRS} -I${include}")
    endforeach()

    get_if_link_libraries(libraries link_dirs ${TARGET})

    if(NOT Argument_NO_DEFAULT_LIB_DIR_FILTER)
        # remove the default library dir e.g /usr/lib
        list(LENGTH link_dirs _link_dirs_count)
        if (_link_dirs_count GREATER 0)
            list(REMOVE_ITEM link_dirs "${CMAKE_INSTALL_PREFIX}/${TARGET_LIBRARY_DIR}")
        endif()
    endif()

    foreach(directory ${link_dirs})
        set (LIBRARIES "${LIBRARIES} -L${directory}")
    endforeach()

    foreach(library ${libraries})
        # Maybe its a linker flag
        if(library MATCHES "^-u" OR library MATCHES "^-l" OR library MATCHES "^-s")
            set (LIBRARIES "${LIBRARIES} ${library}")
        else()
            set (LIBRARIES "${LIBRARIES} -l${library}")
        endif()
    endforeach()

    message(STATUS "${TARGET} added support for generic consumers via ${_pc_filename}")

    configure_file( "${_pc_template}"
                    "${CMAKE_CURRENT_BINARY_DIR}/${_pc_filename}"
                    @ONLY)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_pc_filename}"
            DESTINATION "${_install_path}")
endfunction(HeaderOnlyInstallPackageConfig)
