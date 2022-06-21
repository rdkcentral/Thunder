# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
                    message(SEND_ERROR "${target} wants ${_library} is assumed to be known cmake Target, but it's not known in this scope, maybe you forgot a find_package")
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

    foreach(_target ${Argument_TARGETS})
        if(NOT TARGET ${_target})
            message(SEND_ERROR "There is no target named '${_target}'")
            break()
        endif()

        get_target_property(_type ${_target} TYPE)

        if(NOT "${_type}" STREQUAL "INTERFACE_LIBRARY")
            get_target_property(_version ${_target} VERSION)
            get_target_property(_name ${_target} OUTPUT_NAME)
            get_target_property(_dependencies ${_target} INTERFACE_LINK_LIBRARIES)
        endif()

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

        if(NOT "${_type}" STREQUAL "INTERFACE_LIBRARY")
            # The alias is used by local targets project
            add_library(${_name}::${_name} ALIAS ${_target})

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
                                unset(_dep_name)
                            endif()
                        endif()
                        get_target_property(_comp_iface ${_dependency} COMPATIBLE_INTERFACE_STRING)
                   
                        if(_comp_iface)
                            foreach(_target ${_comp_iface})
                                if("${_target}" MATCHES "${NAMESPACE}")
                                    list(APPEND dependencies  ${_target})
                                endif()  
                            endforeach()
                        endif()
                    endif()
                endforeach()
            endif()
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

function(semicolon_safe_string StringVar)
  string(ASCII 31 semicolon_code)
  string(REPLACE  ";" "${semicolon_code}" SafeString "${${StringVar}}")
  set(${StringVar} "${SafeString}" PARENT_SCOPE) 
endfunction(semicolon_safe_string)

function(InstallCompatibleCMakeConfig)
    set(optionsArgs SKIP_PUBLIC_HEADERS)
    set(oneValueArgs TARGET LEGACY_TARGET LEGACY_PUBLIC_HEADER_LOCATION LEGACY_INCLUDE_DIR)
    set(multiValueArgs)

    cmake_parse_arguments(Arg "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(Argument_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown keywords given to InstallCMakeConfig(): \"${Arg_UNPARSED_ARGUMENTS}\".")
    endif()

    if(NOT Arg_LEGACY_TARGET OR NOT Arg_TARGET)
        message(FATAL_ERROR "Missing keywords LEGACY_TARGET OR TARGET.")
    endif()

    if(NOT TARGET ${Arg_TARGET})
        message(FATAL_ERROR "Target ${Arg_TARGET} is not a defined cmake target in this scope.")
    endif()

    if("${Arg_LEGACY_PUBLIC_HEADER_LOCATION}" STREQUAL "")
        set(Arg_LEGACY_PUBLIC_HEADER_LOCATION "include")
        message(STATUS "LEGACY_PUBLIC_HEADER_LOCATION was set to default '${Arg_LEGACY_PUBLIC_HEADER_LOCATION}'.")
    endif()

    if("${Arg_LEGACY_INCLUDE_DIR}" STREQUAL "")
       set(Arg_LEGACY_INCLUDE_DIR "include")
        message(STATUS "LEGACY_INCLUDE_DIR was set to default '${Arg_LEGACY_INCLUDE_DIR}'.")
    endif()

    add_library(${Arg_LEGACY_TARGET} INTERFACE)
    add_library(${Arg_LEGACY_TARGET}::${Arg_LEGACY_TARGET} ALIAS ${Arg_LEGACY_TARGET})

    target_link_libraries(${Arg_LEGACY_TARGET}
                        INTERFACE ${Arg_TARGET}::${Arg_TARGET})

    if(NOT Arg_SKIP_PUBLIC_HEADERS)
        get_target_property(_headers ${Arg_TARGET} PUBLIC_HEADER)
    
        set_target_properties(${Arg_LEGACY_TARGET} PROPERTIES
                        PUBLIC_HEADER "${_headers}" # specify the public headers
                        )
    endif()

    set_target_properties(${Arg_LEGACY_TARGET} PROPERTIES
                        COMPATIBLE_INTERFACE_STRING "${Arg_LEGACY_TARGET}"
                        )
    install(
            TARGETS ${Arg_LEGACY_TARGET}
            EXPORT ${Arg_LEGACY_TARGET}Targets
            PUBLIC_HEADER DESTINATION ${Arg_LEGACY_PUBLIC_HEADER_LOCATION} COMPONENT devel
            INCLUDES DESTINATION  ${Arg_LEGACY_INCLUDE_DIR} # default include path
    )

    installcmakeconfig(TARGETS ${Arg_LEGACY_TARGET} EXTRA_DEPENDENCIES  ${Arg_TARGET})
endfunction(InstallCompatibleCMakeConfig)
