
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_APPS "Build cdroid custom apps" ON)
option(BUILD_CDROID_TESTS "Build unit tests" ON)

option(ENABLE_GIF "enable gif encode and decoder" OFF)
option(ENABLE_JPEG "enable jpeg decoder" ON)
option(ENABLE_WEBP "enable webp decoder" ON)
option(ENABLE_PLPLOT "Enable PLPLot" OFF)
option(ENABLE_AUDIO "Enabled Audio(Sound Effect)" OFF)
option(ENABLE_BARCODE "Enable BarCode(QrCode Code11 Code49 Code93...)" ON)
option(ENABLE_QRCODE "Enable QRCode(only support QRCode)" ON)
option(ENABLE_LOTTIE "Enable Lottie Animation" OFF)
option(ENABLE_LCMS "Enable Little CMS (a color management engine)" OFF)
option(ENABLE_FRIBIDI "Enable BiDi layout" ON)
option(FT_WITH_HARFBUZZ "Improve auto-hinting of OpenType fonts." OFF)

option(ENABLE_PINYIN2HZ "Chinese Pinyin to HZ support" ON)
option(ENABLE_DIALOGS "Enable AlertDialogs" ON)
option(ENABLE_SPINNER "Enable Spinner" ON)
option(ENABLE_DAYTIME_WIDGETS "Enable Daytime widgets" OFF)
option(ENABLE_RECYCLERVIEW "Enable RecyclerView" ON)
option(ENABLE_NAVIGATION "Enable Navigation" OFF)

find_package(ZIP REQUIRED)
find_package(Freetype2 REQUIRED)
find_package(EXPAT REQUIRED)
find_package(Cairo REQUIRED)
find_package(LUNASVG)
find_package(Fontconfig REQUIRED)
find_package(UniBreak REQUIRED)
find_package(litehtml CONFIG)
find_package(RLOTTIE)
#find_package(PLPLOT)
find_package(zint CONFIG) #barcode generater
find_package(Fribidi)
find_package(RTAUDIO)

set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${RTAUDIO_INCLUDE_DIRS}")
#message(FATAL_ERROR "Matplot++=${Matplot++_LIBRARY}-${Matplot++_INCLUDE_DIR} -${Matplot++_FOUND}-${Matplot++_LIBRARIES}")

list(APPEND CDROID_DEPLIBS
    ${PNG_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${UNIBREAK_LIBRARIES}
    ${ZIP_LIBRARIES}
)

if(RTAUDIO_FOUND AND ENABLE_AUDIO)
    list(APPEND CDROID_DEPLIBS ${RTAUDIO_LIBRARIES})
else()
    set(ENABLE_AUDIO OFF)
endif()

if(RLOTTIE_FOUND AND ENABLE_LOTTIE)
    list(APPEND CDROID_DEPLIBS ${RLOTTIE_LIBRARIES})
else()
   set(ENABLE_LOTTIE OFF)
endif()

if (litehtml_FOUND)
    list( APPEND CDROID_DEPLIBS litehtml)
    #list(APPEND CDROID_DEPINCLUDES ${LITEHTML_INCLUDE_DIRS})
    #add_definitions(-DENABLE_LITEHTML=1)
endif()

if (PLPLOT_FOUND)
    list( APPEND CDROID_DEPLIBS ${PLPLOT_LIBRARIES})
    list(APPEND CDROID_DEPINCLUDES ${PLPLOT_INCLUDE_DIRS})
    add_definitions(-DENABLE_PLPLOT=1)
endif()

if (ENABLE_BARCODE AND zint_FOUND)
    list( APPEND CDROID_DEPLIBS zint::zint)
    #list(APPEND CDROID_DEPINCLUDES ${ZINT_INCLUDE_DIRS})
else()
    set(ENABLE_BARCODE OFF)
endif()

if(ENABLE_FRIBIDI OR FRIBIDI_FOUND)
    find_package(Fribidi REQUIRED)
    list(APPEND CDROID_DEPINCLUDES ${FRIBIDI_INCLUDE_DIRS})
    list(APPEND CDROID_DEPLIBS ${FRIBIDI_LIBRARIES})
else()
    set(ENABLE_FRIBIDI OFF)
endif(ENABLE_FRIBIDI)

list(APPEND CDROID_DEPINCLUDES
    ${ZIP_INCLUDE_DIRS}
    ${EXPAT_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
)

message("CDROID_DEPLIBS=${CDROID_DEPLIBS}")

if(ENABLE_PINYIN2HZ)
    list(APPEND OPTIONAL_LIBS pinyin)
    list(APPEND CDROID_DEPLIBS pinyin)
    list(APPEND CDROID_DEPINCLUDES ${CMAKE_SOURCE_DIR}/src/3rdparty/pinyin/include)
endif()

set(SKIP_INSTALL_EXPORT TRUE)

if(NOT VCPKG_TARGET_TRIPLET)
foreach(lib ${CDROID_DEPLIBS})
    get_filename_component(libpath ${lib} DIRECTORY)
    set(linkdone FALSE)
    while(NOT linkdone AND EXISTS ${lib})
       execute_process( COMMAND readlink ${lib} OUTPUT_VARIABLE linkfile  OUTPUT_STRIP_TRAILING_WHITESPACE)
       get_filename_component(libname ${lib} NAME)
       get_filename_component(linkpath "${linkfile}" DIRECTORY)
       get_filename_component(linkname "${linkfile}" NAME)
       if("${linkpath}" STREQUAL "")
	       set(linkpath ${libpath})
       endif()
       if("${linkfile}" STREQUAL "")
           install(FILES ${lib} DESTINATION lib)
       else()
	       get_filename_component(fromfile ${linkfile} NAME)
	       install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
	          ${fromfile} ${libname} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/lib)")
       endif("${linkfile" STREQUAL "")
       set(lib ${linkpath}/${linkname})
       if("${linkfile}" STREQUAL "")
           set(linkdone TRUE)
       endif()
    endwhile()
endforeach(lib ${CDROID_DEPLIBS})
endif(NOT VCPKG_TARGET_TRIPLET)
