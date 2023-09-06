find_package(PkgConfig)
pkg_check_modules(PC_PIXMAN  pixman-1)

find_path(PIXMAN_INCLUDE_DIRS
    NAMES pixman.h
    HINTS ${PC_PIXMAN_INCLUDEDIR}
    ${PC_PIXMAN_INCLUDE_DIRS}
)

find_library(PIXMAN_LIBRARIES
    NAMES pixman-1
    HINTS ${PC_PIXMAN_LIBDIR}
    ${PC_PIXMAN_LIBRARY_DIRS}
)

#message("PIXMAN_LIBRARIES=${PIXMAN_LIBRARIES} PIXMAN_INCLUDE_DIRS=${PIXMAN_INCLUDE_DIRS} PC_PIXMAN_VERSION=${PC_PIXMAN_VERSION}")
if(PIXMAN_INCLUDE_DIRS AND PIXMAN_LIBRARIES)
    set(PIXMAN_FOUND TRUE)
    set(PIXMAN_LIBRARY ${PIXMAN_LIBRARIES})
    set(PIXMAN_INCLUDE_DIR ${PIXMAN_INCLUDE_DIRS})
    set(PIXMAN_VERSION ${PC_PIXMAN_VERSION})
endif()

include(FindPackageHandleStandardArgs)
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(Pixman REQUIRED_VARS PIXMAN_INCLUDE_DIRS PIXMAN_LIBRARIES 
#	FOUND_VAR PIXMAN_FOUND VERSION_VAR PIXMAN_VERSION)

mark_as_advanced(PIXMAN_LIBRARIES PIXMAN_LIBRARY PIXMAN_INCLUDE_DIRS PIXMAN_INCLUDE_DIR PIXMAN_FOUND)

