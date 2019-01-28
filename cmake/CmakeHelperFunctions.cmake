include (CMakePackageConfigHelpers)

macro(add_element list element)
    # message(SEND_ERROR "Adding '${element}' to list '${list}'")
    list(APPEND ${list} ${element})
endmacro()

function(_get_default_link_name lib name location) 
    string(REPLACE ${CMAKE_SYSROOT} "" _rel_location ${lib})

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
            
            list(LENGTH includes _includes_count)

            if (_includes_count GREATER 0)
                list(GET _configurations 0 _config)
                set (config _${_config})
            endif()
            
            get_target_property(_lib_loc ${target} IMPORTED_LOCATION${config})
            _get_default_link_name(${_lib_loc} _name _dir)
        else()
            get_target_property(_version ${target} VERSION)
            get_target_property(_name ${target} OUTPUT_NAME)

            if(NOT _name)
                get_target_property(_name ${target} NAME)
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
        
        list(LENGTH includes _includes_count)

        if (_includes_count GREATER 0)
            list(GET _configurations 0 _config)
            set (config _${_config})
        endif()
        
        get_target_property(_location ${target} IMPORTED_LOCATION${config})

        if (NOT "${_location}" MATCHES "^.*NOTFOUND")
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
    
    if (NOT "${_if_link_libraries}" MATCHES "^.*NOTFOUND")
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

    list(REMOVE_DUPLICATES _link_libraries)

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

    if (NOT "${_if_compile_defines}" MATCHES "^.*NOTFOUND")
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
    
    if (NOT "${_if_link_libraries}" MATCHES "^.*NOTFOUND")
        foreach(_link ${_if_link_libraries})
            if(TARGET ${_link})
                get_if_compile_defines(_compile_defines ${_link})
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES _compile_defines)

    set(${_result} ${_compile_defines} PARENT_SCOPE )
endfunction()

function(get_if_compile_options _result _target)
    if(NOT TARGET ${_target})
      message(SEND_ERROR "There is no target named '${_target}'")
      return()
    endif()

    set(_compile_options "${${_result}}" )

    get_target_property(_if_compile_options ${_target} INTERFACE_COMPILE_OPTIONS)

    if (NOT "${_if_compile_options}" MATCHES "^.*NOTFOUND")
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
    
    if (NOT "${_if_link_libraries}" MATCHES "^.*NOTFOUND")
        foreach(_link ${_if_link_libraries})
            if(TARGET ${_link})
                get_if_compile_options(_compile_options ${_link})
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES _compile_options)

    set(${_result} ${_compile_options} PARENT_SCOPE)
endfunction() 

function(get_if_include_dirs _result _target)
    if(NOT TARGET ${_target})
      message(SEND_ERROR "There is no target named '${_target}'")
      return()
    endif()

    set(_include_dirs "${${_result}}")

    get_target_property(_if_include_dirs ${_target} INTERFACE_INCLUDE_DIRECTORIES)

    if (NOT "${_if_include_dirs}" MATCHES "^.*NOTFOUND")
        foreach(_include ${_if_include_dirs})
            if(TARGET ${_include})
               get_if_include_dirs(__include_dirs ${_include})
               set(_include_dirs "${_include_dirs} ${__include_dirs}")
            else()
                if (NOT "${_include}" MATCHES "BUILD_INTERFACE")
                    string(REGEX REPLACE "\\$<INSTALL_INTERFACE:" "" __include ${_include})
                    string(REGEX REPLACE ">$" "" ___include ${__include})
                    string(REPLACE ${CMAKE_SYSROOT} "" ____include ${___include})

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
    
    if (NOT "${_if_link_libraries}" MATCHES "^.*NOTFOUND")
        foreach(_link ${_if_link_libraries})
            if(TARGET ${_link})
                get_if_include_dirs(_include_dirs ${_link})
            endif()
        endforeach()
    endif()

    list(REMOVE_DUPLICATES _include_dirs)

    set(${_result} ${_include_dirs} PARENT_SCOPE)
endfunction()

function(InstallCMakeConfig)
    set(optionsArgs)
    set(oneValueArgs LOCATION)
    set(multiValueArgs TARGETS)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    set(_install_path "lib/cmake") # default path

    if(Agument_LOCATION)
        set(_install_path "${Agument_LOCATION}" FORCE)
    endif()

    foreach(_target ${Argument_TARGETS})
        if(NOT TARGET ${_target})
            message(SEND_ERROR "There is no target named '${_target}'")
            break()
        endif()

        get_target_property(_version ${_target} VERSION)
        get_target_property(_name ${_target} OUTPUT_NAME)
    
        if(NOT _name)
            get_target_property(_name ${_target} NAME)
        endif()

        if ("${_name}" MATCHES "^.*NOTFOUND")
            message(FATAL_ERROR "right... ${_target} should have a name right, time for coffee?!")
        endif()

        write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake
            VERSION ${_version}
            COMPATIBILITY SameMajorVersion)

        message(STATUS "${_target} added support for cmake consumers via '${_install_path}/${_name}Config.cmake'")

        add_library(${_name}::${_name} ALIAS ${_target})

        install(EXPORT "${_target}Targets"
            FILE "${_name}Config.cmake"
            NAMESPACE  "${_name}::"
            DESTINATION "${_install_path}/${_name}")

        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_name}ConfigVersion.cmake"
            DESTINATION "${_install_path}/${_name}")
    endforeach()
endfunction(InstallCMakeConfig)

function(InstallPackageConfig)
    set(optionsArgs NO_DEFAULT_LIB_DIR_FILTER NO_COMPILE_OPTIONS NO_INCLUDE_DIRS NO_COMPILE_DEFINES)
    set(oneValueArgs TEMPLATE DESCRIPTION LOCATION OUTPUT_NAME)
    set(multiValueArgs TARGETS)

    cmake_parse_arguments(Argument "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if("${Argument_TEMPLATE}" STREQUAL "")
        find_file( _pc_template
                    NAMES "default.pc.in"
                    PATHS ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_SYSROOT}/usr/include/${NAMESPACE}/cmake
                    NO_DEFAULT_PATH
                    NO_CMAKE_ENVIRONMENT_PATH
                    NO_CMAKE_PATH
                    NO_SYSTEM_ENVIRONMENT_PATH
                    NO_CMAKE_SYSTEM_PATH
                    NO_CMAKE_FIND_ROOT_PATH)

        find_file(_pc_template  
                    NAMES "default.pc.in"
                    PATHS ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_SYSROOT}/usr/include/${NAMESPACE}/cmake )

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
        string(REPLACE ${CMAKE_SYSROOT}/ "" TARGET_LIBRARY_DIR ${Argument_LOCATION})
        string(REPLACE "${CMAKE_INSTALL_PREFIX}/" "" TARGET_LIBRARY_DIR ${TARGET_LIBRARY_DIR})
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

        if ("${NAME}" MATCHES "^.*NOTFOUND")
            message(FATAL_ERROR "right... ${_target} should have a name right, time for coffee?!")
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
            set (LIBRARIES "${LIBRARIES} -l${library}")
        endforeach()

        message(STATUS "${_target} added support for generic consumers via ${_pc_filename}")

        configure_file( "${_pc_template}"
                        "${CMAKE_CURRENT_BINARY_DIR}/${_pc_filename}"
                        @ONLY)

        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${_pc_filename}"
                DESTINATION "${_install_path}")
    endforeach()
endfunction(InstallPackageConfig)

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