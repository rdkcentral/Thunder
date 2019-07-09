include (CMakePackageConfigHelpers)

macro(add_element list element)
    # message(SEND_ERROR "Adding '${element}' to list '${list}'")
    list(APPEND ${list} ${element})
endmacro()

function(_get_default_link_name lib name location) 
    string(REPLACE "${CMAKE_SYSROOT}" "" _rel_location ${lib})

    get_filename_component(_extention "${_rel_location}" EXT)
    get_filename_component(_name "${_rel_location}" NAME)
    get_filename_component(_location "${_rel_location}" DIRECTORY)
    
    if(NOT "${_location}" STREQUAL "")   
    	string(REGEX REPLACE "^${CMAKE_INSTALL_PREFIX}/lib" "" _location ${_location})
        get_filename_component(_location "${_rel_location}" DIRECTORY)
    endif()

    if(NOT "${_extention}" STREQUAL "")
        get_filename_component(_name "${_rel_location}" NAME_WE)
        string(REGEX REPLACE "^lib" "" _name ${_name})
    endif()
    
    set(${name} ${_name} PARENT_SCOPE)
    set(${location} ${_location} PARENT_SCOPE)
endfunction()

function(get_if_link_libraries libs dirs target)
    if(NOT TARGET ${target})
      message(SEND_ERROR "There is no target named '${target}'")
      return()
    endif()

    set(_link_libraries "${${libs}}")
    set(_link_dirs "${${dirs}}")

    get_target_property(_type ${target} TYPE)
    get_target_property(_is_imported ${target} IMPORTED)

    if(NOT _type)
        # this should never happen in a normal situation.... 
        message(FATAL_ERROR "hmmmm...  ${target} is typeless... double check it's \"add_library\" ")
    endif()

    # maybe we need to add this target.... 
    if("${_type}" MATCHES "SHARED_LIBRARY" OR "${_type}" MATCHES "STATIC_LIBRARY")
        if(_is_imported)
            get_target_property(_configurations ${target} IMPORTED_CONFIGURATIONS)
            
            if (_configurations)
                list(LENGTH _configurations _configurations_count)

                list(GET _configurations 0 _config)
                set (config _${_config})

                if (_configurations_count GREATER 1)
                    message(AUTHOR_WARNING "Multiple configs not yet supported, got ${_configurations} and picked the first one")
                endif()
            endif()
            
            get_target_property(_lib_loc ${target} IMPORTED_LOCATION${config})
            _get_default_link_name(${_lib_loc} _name _dir)
        else()
            get_target_property(_version ${target} VERSION)
            get_target_property(_name ${target} OUTPUT_NAME)

            if(NOT _name)
                get_target_property(_name ${target} NAME)
            endif()

            if(NOT _version)
                set(_version 0.0.0)
            endif()
        endif(_is_imported)
        
        if(NOT "${_dir}" STREQUAL "")
            set(_link_dirs ${_link_dirs} ${_dir})
        endif()

        if(_name)
            # set(_link_libraries ${_link_libraries} ${_lib_name})
            add_element( _link_libraries  ${_name})
        else()
            # this should never happen in a normal situation....
            message(FATAL_ERROR "right... ${_library} should have a name right, time for coffee?!")
        endif(_name) 
    elseif("${_type}" MATCHES "UNKNOWN_LIBRARY")
        # this lib is via a findmodule imported, grab the imported location  asuming it's fins script is accourdng guidelines
        # we remove the absolute systroot to make it relative. Later we can decide to use the path in a -L argument <;-)
        get_target_property(_configurations ${target} IMPORTED_CONFIGURATIONS)
        
        if (_configurations)
            list(LENGTH _configurations _configurations_count)

            list(GET _configurations 0 _config)
            set (config _${_config})

            if (_configurations_count GREATER 1)
                message(AUTHOR_WARNING "Multiple configs not yet supported, got ${_configurations} and picked the first one")
            endif()
        endif()
        
        get_target_property(_location ${target} IMPORTED_LOCATION${config})

        if (_location)
                _get_default_link_name(${_location} _name _dir)

                if(NOT "${_dir}" STREQUAL "")
                    set(_link_dirs ${_link_dirs} ${_dir})
                endif()

                set(_link_libraries ${_link_libraries} ${_name})
        else()
                message(AUTHOR_WARNING "Skipping ${_library} it's find module needs probably love, the IMPORTED_LOCATION${config} is not set properly...")
        endif()
    endif()

    # Check its public dependecies
    get_target_property(_if_link_libraries ${target} INTERFACE_LINK_LIBRARIES)
    
    if (_if_link_libraries)
        foreach(_library  ${_if_link_libraries})
            if ("${_library}" MATCHES "LINK_ONLY")
                string(REGEX REPLACE "\\$<LINK_ONLY:" "" __fix ${_library})
                string(REGEX REPLACE ">$" "" _library ${__fix})
            endif()

            if(TARGET ${_library})
                # message(SEND_ERROR "Checking target ${_library}")
                get_if_link_libraries(_link_libraries _link_dirs ${_library})
            else()
                if ("${_library}" MATCHES "^.*\\:\\:.*$")
                     # detecting one or more "::" this is normally a cmake target 
                    message(SEND_ERROR "${_library} is assumed to be known cmake Target, but it's not known in this scope, maybe you forgot a find_package")
                else()
                    # set(_link_libraries ${_link_libraries} ${_library})
                    add_element(_link_libraries  ${_library})
                endif()
            endif()
        endforeach()
    endif()

    if(_link_libraries)
        list(REMOVE_DUPLICATES _link_libraries)
    endif()

    if(_link_dirs)
        list(REMOVE_DUPLICATES _link_dirs)
    endif()

    set(${libs} ${_link_libraries} PARENT_SCOPE)
    set(${dirs} ${_link_dirs} PARENT_SCOPE)
endfunction()

function(get_if_compile_defines _result _target)
    if(NOT TARGET ${_target})
      message(SEND_ERROR "There is no target named '${_target}'")
      return()
    endif()

    set(_compile_defines "${${_result}}")

    get_target_property(_if_compile_defines ${_target} INTERFACE_COMPILE_DEFINITIONS)

    if (_if_compile_defines)
        foreach(_define ${_if_compile_defines})
            if(TARGET ${_define})
                get_if_compile_defines(_compile_defines ${_define})
            else()
                #set(_compile_defines ${_compile_defines} ${_define})
                add_element(_compile_defines ${_define})
            endif()
        endforeach()
    endif()

    get_target_property(_if_link_libraries ${_target} INTERFACE_LINK_LIBRARIES) 
    
    if (_if_link_libraries)
        foreach(_link ${_if_link_libraries})
            if(TARGET ${_link})
                get_if_compile_defines(_compile_defines ${_link})
            endif()
        endforeach()
    endif()

    if(_compile_defines)
        list(REMOVE_DUPLICATES _compile_defines)
    endif()
    
    set(${_result} ${_compile_defines} PARENT_SCOPE )
endfunction()

function(get_if_compile_options _result _target)
    if(NOT TARGET ${_target})
      message(SEND_ERROR "There is no target named '${_target}'")
      return()
    endif()

    set(_compile_options "${${_result}}" )

    get_target_property(_if_compile_options ${_target} INTERFACE_COMPILE_OPTIONS)

    if (_if_compile_options)
        foreach(_option ${_if_compile_options})
            if(TARGET ${_option})
                get_if_compile_options(_compile_options ${_option})
            else()
                # set(_compile_options ${_compile_options} ${_option})
                add_element(_compile_options ${_option})
            endif()
        endforeach()
    endif()

    get_target_property(_if_link_libraries ${_target} INTERFACE_LINK_LIBRARIES) 
    
    if (_if_link_libraries)
        foreach(_link ${_if_link_libraries})
            if(TARGET ${_link})
                get_if_compile_options(_compile_options ${_link})
            endif()
        endforeach()
    endif()

    if(_compile_options)
        list(REMOVE_DUPLICATES _compile_options)
    endif()

    set(${_result} ${_compile_options} PARENT_SCOPE)
endfunction() 

function(get_if_include_dirs _result _target)
    if(NOT TARGET ${_target})
      message(SEND_ERROR "There is no target named '${_target}'")
      return()
    endif()

    set(_include_dirs "${${_result}}")

    get_target_property(_if_include_dirs ${_target} INTERFACE_INCLUDE_DIRECTORIES)

    if (_if_include_dirs)
        foreach(_include ${_if_include_dirs})
            if(TARGET ${_include})
               get_if_include_dirs(__include_dirs ${_include})
               set(_include_dirs "${_include_dirs} ${__include_dirs}")
            else()
                if (NOT "${_include}" MATCHES "BUILD_INTERFACE")
                    string(REGEX REPLACE "\\$<INSTALL_INTERFACE:" "" __include ${_include})
                    string(REGEX REPLACE ">$" "" ___include ${__include})
                    string(REPLACE "${CMAKE_SYSROOT}" "" ____include ${___include})

                    string(REGEX MATCH "^/.*" _is_absolute_path "${____include}")
                    if(_is_absolute_path)
                        # set(_include_dirs ${_include_dirs} ${___include})
                        add_element(_include_dirs ${____include})
                    else()
                        # set(_include_dirs ${_include_dirs} "${CMAKE_INSTALL_PREFIX}/${___include}")
                        add_element(_include_dirs "${CMAKE_INSTALL_PREFIX}/${____include}")
                    endif()
                endif()
            endif()
        endforeach()
    endif()

    get_target_property(_if_link_libraries ${_target} INTERFACE_LINK_LIBRARIES) 
    
    if (_if_link_libraries)
        foreach(_link ${_if_link_libraries})
            if(TARGET ${_link})
                get_if_include_dirs(_include_dirs ${_link})
            endif()
        endforeach()
    endif()

    if(_include_dirs)
        list(REMOVE_DUPLICATES _include_dirs)
    endif()

    set(${_result} ${_include_dirs} PARENT_SCOPE)
endfunction()

function(InstallCMakeConfig)
    set(optionsArgs NO_SKIP_INTERFACE_LIBRARIES)
    set(oneValueArgs LOCATION TEMPLATE)
    set(multiValueArgs TARGETS EXTRA_DEPENDENCIES)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to InstallCMakeConfig(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    set(_install_path "lib/cmake") # default path
    
    if(Agument_LOCATION)
        set(_install_path "${Agument_LOCATION}" FORCE)
    endif()

    if("${Argument_TEMPLATE}" STREQUAL "")
        find_file( _config_template
            NAMES "defaultConfig.cmake.in"
            PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates"
            NO_DEFAULT_PATH
            NO_CMAKE_ENVIRONMENT_PATH
            NO_CMAKE_PATH
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_CMAKE_SYSTEM_PATH
            NO_CMAKE_FIND_ROOT_PATH)

        find_file(_config_template  
            NAMES "defaultConfig.cmake.in"
            PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates" )
            
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

    foreach(_target ${Argument_TARGETS})
        if(NOT TARGET ${_target})
            message(SEND_ERROR "There is no target named '${_target}'")
            break()
        endif()

        get_target_property(_version ${_target} VERSION)
        get_target_property(_name ${_target} OUTPUT_NAME)
        get_target_property(_dependencies ${_target} INTERFACE_LINK_LIBRARIES)
    
        if(NOT _name)
            get_target_property(_name ${_target} NAME)
        endif()

        if (NOT _name)
            message(FATAL_ERROR "right... ${_target} should have a name right, time for coffee?!")
        endif()

        if(NOT _version)
            set(_version 0.0.0)
        endif()

        write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake
            VERSION ${_version}
            COMPATIBILITY SameMajorVersion)

        message(STATUS "${_target} added support for cmake consumers via '${_name}Config.cmake'")

        # The alias is used by local targets project
        add_library(${_name}::${_name} ALIAS ${_target})

        if(_dependencies)
            foreach(_dependency ${_dependencies})
                if ("${_dependency}" MATCHES "LINK_ONLY")
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
                    
                    if (_type_is_ok)
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

        configure_file( "${_config_template}"
                "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake"
                @ONLY)

        install(EXPORT "${_target}Targets"
            FILE "${_name}Targets.cmake"
            NAMESPACE  "${_name}::"
            DESTINATION "${_install_path}/${_name}")

        install(FILES 
                "${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/${_name}Config.cmake"
            DESTINATION "${_install_path}/${_name}")
    endforeach()
endfunction(InstallCMakeConfig)

function(InstallPackageConfig)
    set(optionsArgs NO_DEFAULT_LIB_DIR_FILTER NO_COMPILE_OPTIONS NO_INCLUDE_DIRS NO_COMPILE_DEFINES)
    set(oneValueArgs TEMPLATE DESCRIPTION LOCATION OUTPUT_NAME)
    set(multiValueArgs TARGETS)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to InstallPackageConfig(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    if("${Argument_TEMPLATE}" STREQUAL "")
        find_file( _pc_template
                    NAMES "default.pc.in"
                    PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates"
                    NO_DEFAULT_PATH
                    NO_CMAKE_ENVIRONMENT_PATH
                    NO_CMAKE_PATH
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_FIND_ROOT_PATH)

        find_file(_pc_template  
                    NAMES "default.pc.in"
                    PATHS "${PROJECT_SOURCE_DIR}/cmake/templates" "${CMAKE_SYSROOT}/usr/lib/cmake/${NAMESPACE}/templates" )

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

    set(DESCRIPTION "${Argument_DESCRIPTION}")

    if (${Argument_LOCATION})
        string(REPLACE "${CMAKE_SYSROOT}" "" TARGET_LIBRARY_DIR ${Argument_LOCATION})
        string(REPLACE "${CMAKE_INSTALL_PREFIX}" "" TARGET_LIBRARY_DIR ${TARGET_LIBRARY_DIR})
        string(REGEX REPLACE "^(/+)" "" TARGET_LIBRARY_DIR ${TARGET_LIBRARY_DIR})
    else()
        set(TARGET_LIBRARY_DIR "lib")
    endif()

    list(LENGTH Argument_TARGETS _target_count)

    if (${Argument_OUTPUT_NAME})
        if (_target_count GREATER 1 )
            message(SEND_ERROR "Can't use OUTPUT_NAME with multiple targets... yet.")
        endif()
    endif()

    foreach(_target ${Argument_TARGETS})
        if(NOT TARGET ${_target})
            message(SEND_ERROR "There is no target named '${_target}'")
            break()
        endif()

        get_target_property(VERSION ${_target} VERSION)
        get_target_property(NAME ${_target} OUTPUT_NAME)
    
        if(NOT NAME)
            get_target_property(NAME ${_target} NAME)
        endif()

        if (NOT NAME)
            message(FATAL_ERROR "right... ${_target} should have a name right, time for coffee?!")
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
            get_if_compile_options(compile_options ${_target})
            foreach(compile_option ${compile_options})
                set (COMPILE_OPTIONS "${COMPILE_OPTIONS} ${compile_option}")
            endforeach()
        endif()

        if(NOT Argument_NO_COMPILE_DEFINES)
            get_if_compile_defines(defines ${_target})
            foreach(define ${defines})
                set (COMPILE_DEFINES "${COMPILE_DEFINES} -D${define}")
            endforeach()
        endif()

        if(NOT Argument_NO_INCLUDE_DIRS)
            get_if_include_dirs(includes ${_target})
            list(LENGTH includes _includes_count)

            if (_includes_count GREATER 0)
                # Assuming the first in the list belongs to _target
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

        get_if_link_libraries(libraries link_dirs ${_target})

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

        message(STATUS "${_target} added support for generic consumers via ${_pc_filename}")

        configure_file( "${_pc_template}"
                        "${CMAKE_CURRENT_BINARY_DIR}/${_pc_filename}"
                        @ONLY)

        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_pc_filename}"
                DESTINATION "${_install_path}")
    endforeach()
endfunction(InstallPackageConfig)

function(InstallFindModule)
    set(optionsArgs RECURSE)
    set(multiValueArgs FILES)
    set(oneValueArgs DIRECTORY)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(DESTINATION lib/cmake/${NAMESPACE}/modules) 

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to InstallCMakeConfig(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    if(Argument_RECURSE)
        if(Argument_RECURSE)
            file(GLOB extra_files "${DIRECTORY}/find*.cmake")
        else()
            file(GLOB_RECURSE extra_files "${DIRECTORY}/*.cmake")
        endif(Argument_RECURSE)
        install(FILES "${extra_files}" DESTINATION lib/cmake/${NAMESPACE}/modules)
    endif()

    if (Argument_FILES)
        install(FILES "${Argument_FILES}" DESTINATION lib/cmake/${NAMESPACE}/modules)
    endif()

endfunction(InstallFindModule)

# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

function(print_properties)
    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
endfunction(print_properties)

function(print_target_properties tgt)
    if(NOT TARGET ${tgt})
      message("There is no target named '${tgt}'")
      return()
    endif()

    foreach (prop ${CMAKE_PROPERTY_LIST})
        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
    # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
    if(prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
        continue()
    endif()
        # message ("Checking ${prop}")
        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
        if (propval)
            get_target_property(propval ${tgt} ${prop})
            message ("${tgt} ${prop} = ${propval}")
        endif()
    endforeach(prop)
endfunction(print_target_properties)

function(JsonGenerator)
    if (NOT JSON_GENERATOR)
        message(FATAL_ERROR "The path JSON_GENERATOR is not set!")
    endif()

    if(NOT EXISTS "${JSON_GENERATOR}" OR IS_DIRECTORY "${JSON_GENERATOR}")
        message(FATAL_ERROR "JsonGenerator path ${JSON_GENERATOR} invalid.")
    endif()

    set(optionsArgs CODE STUBS DOCS NO_WARNINGS COPY_CTOR NO_REF_NAMES)
    set(oneValueArgs OUTPUT IFDIR INDENT DEF_STRING DEF_INT_SIZE PATH)
    set(multiValueArgs INPUT)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to JsonGenerator(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(_execute_command ${JSON_GENERATOR})

    if(Argument_CODE)
        list(APPEND _execute_command  "--code")
    endif()

    if(Argument_STUBS)
        list(APPEND _execute_command  "--stubs")
    endif()

    if(Argument_DOCS)
        list(APPEND _execute_command  "--docs")
    endif()

    if(Argument_NO_WARNINGS)
        list(APPEND _execute_command  "--no-warnings")
    endif()

    if(Argument_COPY_CTOR)
        list(APPEND _execute_command  "--copy-ctor")
    endif()

    if(Argument_NO_REF_NAMES)
        list(APPEND _execute_command  "--no-ref-names")
    endif()

    if (Argument_PATH)
        list(APPEND _execute_command  "-p" "${Argument_PATH}")
    endif()

    if (Argument_IFDIR)
        list(APPEND _execute_command  "-i" "${Argument_IFDIR}")
    endif()

    if (Argument_OUTPUT)
        file(MAKE_DIRECTORY "${Argument_OUTPUT}")
        list(APPEND _execute_command  "--output" "${Argument_OUTPUT}")
    endif()

    if (Argument_INDENT)
        list(APPEND _execute_command  "--indent" "${Argument_INDENT}")
    endif()

    if (Argument_DEF_STRING)
        list(APPEND _execute_command  "--def-string" "${Argument_DEF_STRING}")
    endif()

    if (Argument_DEF_INT_SIZE)
        list(APPEND _execute_command  "--def-int-size" "${Argument_DEF_INT_SIZE}")
    endif()

    foreach(_input ${Argument_INPUT})
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_execute_command} ${_input} RESULT_VARIABLE rv)
        if(NOT ${rv} EQUAL 0)
            message(FATAL_ERROR "JsonGenerator generator failed.")
        endif()
    endforeach(_input)
endfunction(JsonGenerator)

function(ProxyStubGenerator)
    if (NOT PROXYSTUB_GENERATOR)
        message(FATAL_ERROR "The path PROXYSTUB_GENERATOR is not set!")
    endif()

    if(NOT EXISTS "${PROXYSTUB_GENERATOR}" OR IS_DIRECTORY "${PROXYSTUB_GENERATOR}")
        message(FATAL_ERROR "ProxyStubGenerator path ${PROXYSTUB_GENERATOR} invalid.")
    endif()

    set(optionsArgs SCAN_IDS TRACES OLD_CPP NO_WARNINGS KEEP VERBOSE)
    set(oneValueArgs INCLUDE NAMESPACE INDENT)
    set(multiValueArgs INPUT)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to ProxyStubGenerator(): \"${Argument_UNPARSED_ARGUMENTS}\"")
    endif()

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(_execute_command ${PROXYSTUB_GENERATOR})

    if(Argument_SCAN_IDS)
        list(APPEND _execute_command  "--scan-ids")
    endif()

    if(Argument_TRACES)
        list(APPEND _execute_command  "--traces")
    endif()

    if(Argument_OLD_CPP)
        list(APPEND _execute_command  "--old-cpp")
    endif()

    if(Argument_NO_WARNINGS)
        list(APPEND _execute_command  "--no-warnings")
    endif()

    if(Argument_KEEP)
        list(APPEND _execute_command  "--keep")
    endif()

    if(Argument_VERBOSE)
        list(APPEND _execute_command  "--verbose")
    endif()

    if (Argument_INCLUDE)
        list(APPEND _execute_command  "-i" "${Argument_INCLUDE}")
    endif()

    if (Argument_NAMESPACE)
        list(APPEND _execute_command  "--namespace" "${Argument_NAMESPACE}")
    endif()

    if (Argument_INDENT)
        list(APPEND _execute_command  "--indent" "${Argument_INDENT}")
    endif()

    foreach(_input ${Argument_INPUT})
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${_execute_command} ${_input} RESULT_VARIABLE rv)
        if(NOT ${rv} EQUAL 0)
            message(FATAL_ERROR "ProxyStubGenerator generator failed.")
        endif()
    endforeach(_input)
endfunction(ProxyStubGenerator)
