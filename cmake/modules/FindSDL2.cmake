include(FindPkgConfig)
pkg_check_modules(PC_SDL sdl2)

find_path(SDL2_INCLUDE_DIRS
    NAMES SDL.h
    HINTS ${PC_SDL_INCLUDEDIR}
    ${PC_SDL_INCLUDE_DIRS}
)

find_library(SDL2_LIBRARIES
    NAMES SDL2
    HINTS ${PC_SDL_LIBDIR}
    ${PC_SDL_LIBRARY_DIRS}
)
if(SDL2_INCLUDE_DIRS AND SDL2_LIBRARIES)
    set(SDL2_FOUND TRUE)
    set(SDL2_LIBRARY ${SDL2_LIBRARIES})
    set(SDL2_INCLUDE_DIR ${SDL2_INCLUDE_DIRS})
    set(SDL2_VERSION ${PC_SDL_VERSION})
    include(FindPackageHandleStandardArgs)
    FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2 FOUND_VAR SDL2_FOUND VERSION_VAR SDL2_VERSION
	    REQUIRED_VARS SDL2_INCLUDE_DIRS SDL2_INCLUDE_DIR SDL2_LIBRARIES SDL2_LIBRARY)

    mark_as_advanced(SDL2_LIBRARIES SDL2_LIBRARY SDL2_INCLUDE_DIRS SDL2_INCLUDE_DIR SDL2_VERSION)
endif()

