CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# project
SET(prefix "/usr")
SET(maintainer "Jeongmo Yang<jm80.yang>, Hyuntae Kim<ht1211.kim@samsung.com>, Seokhoon Lee<andy.shlee@samsung.com>, Taeyoung Chung <ty83.chung@samsung.com>, Sejong Park<sejong123.park@samsung.com>")
SET(description "A Camera library in Tizen Native API")
SET(service "mmsvc")
SET(submodule "recorder")
SET(fw_name "${service}-${submodule}")

PROJECT(${fw_name})

SET(CMAKE_INSTALL_PREFIX ${prefix})
SET(PREFIX ${CMAKE_INSTALL_PREFIX})

SET(INC_DIR legacy/include)
INCLUDE_DIRECTORIES(${INC_DIR})
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall -Wextra -Wno-array-bounds -Wno-empty-body -Wno-ignored-qualifiers -Wno-unused-parameter -Wshadow -Wwrite-strings -Wswitch-default -Wno-unused-but-set-parameter -Wno-unused-but-set-variable")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

IF("${ARCH}" STREQUAL "arm")
    ADD_DEFINITIONS("-DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

CONFIGURE_FILE(
    ${fw_name}.pc.in
    ${CMAKE_CURRENT_SOURCE_DIR}/${fw_name}.pc
    @ONLY
)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${fw_name}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

ADD_SUBDIRECTORY(legacy)
ADD_SUBDIRECTORY(muse)

IF(UNIX)

ADD_CUSTOM_TARGET (distclean @echo cleaning for source distribution)
ADD_CUSTOM_COMMAND(
        DEPENDS clean
        COMMENT "distribution clean"
        COMMAND find
        ARGS    .
        -not -name config.cmake -and \(
        -name tester.c -or
        -name Testing -or
        -name CMakeFiles -or
        -name cmake.depends -or
        -name cmake.check_depends -or
        -name CMakeCache.txt -or
        -name cmake.check_cache -or
        -name *.cmake -or
        -name Makefile -or
        -name core -or
        -name core.* -or
        -name gmon.out -or
        -name install_manifest.txt -or
        -name *.pc -or
        -name *~ \)
        | grep -v TC | xargs rm -rf
        TARGET  distclean
        VERBATIM
)

ENDIF(UNIX)
