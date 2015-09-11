CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(service "mused")
SET(submodule "recorder")

SET(dependents "dlog mused mm-common glib-2.0 mmsvc-camera capi-media-audio-io audio-session-mgr")
SET(pc_dependents "dlog capi-media-audio-io")
SET(fw_name "${service}-${submodule}")
PROJECT(${fw_name})

SET(CMAKE_INSTALL_PREFIX /usr)
SET(PREFIX ${CMAKE_INSTALL_PREFIX})

SET(INC_DIR include)
INCLUDE_DIRECTORIES(${INC_DIR})

INCLUDE(FindPkgConfig)
pkg_check_modules(${fw_name} REQUIRED ${dependents})
FOREACH(flag ${${fw_name}_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "-I./include ${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall -Wextra -Wno-array-bounds -Wno-empty-body -Wno-ignored-qualifiers -Wno-unused-parameter -Wshadow -Wwrite-strings -Wswitch-default -Wno-unused-but-set-parameter -Wno-unused-but-set-variable")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

IF("${ARCH}" STREQUAL "arm")
    ADD_DEFINITIONS("-DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS("-DTIZEN_DEBUG")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=${LIB_INSTALL_DIR}")

aux_source_directory(src MUSED_SOURCES)
ADD_LIBRARY(${fw_name} SHARED ${MUSED_SOURCES})


TARGET_LINK_LIBRARIES(${fw_name} mmsvc-recorder ${${fw_name}_LDFLAGS})

SET_TARGET_PROPERTIES(${fw_name}
     PROPERTIES
     VERSION ${FULLVER}
     SOVERSION ${MAJORVER}
     CLEAN_DIRECT_OUTPUT 1
)

INSTALL(TARGETS ${fw_name} DESTINATION ${LIB_INSTALL_DIR})
INSTALL(
        DIRECTORY ${INC_DIR}/ DESTINATION include/media
        FILES_MATCHING
        PATTERN "*_private.h" EXCLUDE
        PATTERN "${INC_DIR}/*.h"
        )

SET(PC_NAME ${fw_name})
SET(PC_REQUIRED ${pc_dependents})
SET(PC_LDFLAGS -l${fw_name})
SET(PC_CFLAGS -I\${includedir}/media)

CONFIGURE_FILE(
    ${fw_name}.pc.in
    ${CMAKE_CURRENT_SOURCE_DIR}/${fw_name}.pc
    @ONLY
)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${fw_name}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
