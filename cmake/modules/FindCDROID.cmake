
find_package(PkgConfig)
pkg_check_modules(PC_CDROID cdroid)
pkg_check_modules(PC_CDHAL cdroidhal)

find_path(CDROID_INCLUDE_DIRS
    NAMES view/view.h
    HINTS ${PC_CDROID_INCLUDEDIR}
          ${PC_CDROID_INCLUDE_DIRS}
    PATH_SUFFIXES gui porting
)
find_path(CDROID_PORTING_DIRS
    NAMES porting/cdgraph.h
    HINTS ${PC_CDROID_INCLUDEDIR}
          ${PC_CDROID_INCLUDE_DIRS}
)

find_library(CDROID_LIBRARIES
    NAMES cdroid
    HINTS ${PC_CDROID_LIBDIR}
          ${PC_CDROID_LIBRARY_DIRS}
)

set(CDROID_INCLUDE_DIRS ${CDROID_INCLUDE_DIRS} ${CDROID_PORTING_DIRS}/porting)
list(APPEND CDROID_LIBRARIES ${PC_CDROID_LDFLAGS} ${CDHAL_LDFLAGS} ${PC_CDHAL_LDFLAGS})
message("...CDROID_LIBRARIES=${CDROID_LIBRARIES} CDROID_INCLUDE_DIRS=${CDROID_INCLUDE_DIRS} ${CDROID_PORTING_DIRS}\n PC_CDROID_LIBDIR=${PC_CDROID_LIBDIR}")

if (CDROID_INCLUDE_DIRS)
    if (EXISTS "${CDROID_INCLUDE_DIRS}/cdroid-version.h")
        file(READ "${CDROID_INCLUDE_DIRS}/cdroid-version.h" CDROID_VERSION_CONTENT)

        string(REGEX MATCH "#define +CDROID_VERSION_MAJOR +([0-9]+)" _dummy "${CDROID_VERSION_CONTENT}")
        set(CDROID_VERSION_MAJOR "${CMAKE_MATCH_1}")

        string(REGEX MATCH "#define +CDROID_VERSION_MINOR +([0-9]+)" _dummy "${CDROID_VERSION_CONTENT}")
        set(CDROID_VERSION_MINOR "${CMAKE_MATCH_1}")

        string(REGEX MATCH "#define +CDROID_VERSION_MICRO +([0-9]+)" _dummy "${CDROID_VERSION_CONTENT}")
        set(CDROID_VERSION_MICRO "${CMAKE_MATCH_1}")

        set(CDROID_VERSION "${CDROID_VERSION_MAJOR}.${CDROID_VERSION_MINOR}.${CDROID_VERSION_MICRO}")
    endif ()
endif ()

if ("${Cdroid_FIND_VERSION}" VERSION_GREATER "${CDROID_VERSION}")
    message(FATAL_ERROR "Required version (" ${Cdroid_FIND_VERSION} ") is higher than found version (" ${CDROID_VERSION} ")")
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CDROID REQUIRED_VARS CDROID_INCLUDE_DIRS CDROID_LIBRARIES
                                        VERSION_VAR CDROID_VERSION)

mark_as_advanced(
    CDROID_INCLUDE_DIRS
    CDROID_LIBRARIES
)

