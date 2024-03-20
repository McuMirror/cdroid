find_package(PkgConfig)
pkg_check_modules(PC_PNG libpng)

find_path(PNG_INCLUDE_DIRS
    NAMES png.h
    HINTS ${PC_PNG_INCLUDEDIR}
    ${PC_PNG_INCLUDE_DIRS}
)

find_library(PNG_LIBRARIES
    NAMES png png12 png16 libpng libpng12 libpng16
    HINTS ${PC_PNG_LIBDIR}
    ${PC_PNG_LIBRARY_DIRS}
)
message("PC_PNG_INCLUDEDIR=${PC_PNG_INCLUDEDIR} PC_PNG_INCLUDE_DIRS=${PC_PNG_INCLUDE_DIRS} PNG_INCLUDE_DIRS=${PNG_INCLUDE_DIRS}")
if(PNG_INCLUDE_DIRS AND PNG_LIBRARIES)
    set(PNG_FOUND TRUE)
    set(PNG_LIBRARY ${PNG_LIBRARIES})
    set(PNG_INCLUDE_DIR ${PNG_INCLUDE_DIRS})
    set(PNG_VERSION ${PC_PNG_VERSION})
    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(PNG FOUND_VAR PNG_FOUND VERSION_VAR PNG_VERSION
	    REQUIRED_VARS PNG_INCLUDE_DIRS PNG_INCLUDE_DIR PNG_LIBRARIES PNG_LIBRARY)

    mark_as_advanced(PNG_LIBRARIES PNG_LIBRARY PNG_INCLUDE_DIRS PNG_INCLUDE_DIR PNG_VERSION)

    if(NOT TARGET PNG::PNG)
        add_library(PNG::PNG UNKNOWN IMPORTED)
        set_target_properties(PNG::PNG PROPERTIES
          INTERFACE_COMPILE_DEFINITIONS "${_PNG_COMPILE_DEFINITIONS}"
          INTERFACE_INCLUDE_DIRECTORIES "${PNG_INCLUDE_DIRS}"
          INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
        if((CMAKE_SYSTEM_NAME STREQUAL "Linux") AND
           ("${PNG_LIBRARY}" MATCHES "\\${CMAKE_STATIC_LIBRARY_SUFFIX}$"))
          set_property(TARGET PNG::PNG APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES m)
        endif()

        if(EXISTS "${PNG_LIBRARY}")
          set_target_properties(PNG::PNG PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "C"
            IMPORTED_LOCATION "${PNG_LIBRARY}")
        endif()
    endif()
endif()

