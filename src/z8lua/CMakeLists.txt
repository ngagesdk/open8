cmake_minimum_required(VERSION 3.10)

option(BUILD_FOR_NOKIA_NGAGE "Build for Nokia N-Gage" OFF)

set(z8lua_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/lapi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lauxlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lbaselib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lbitlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lcode.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lcorolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lctype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldblib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldebug.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ldump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lfunc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lgc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/linit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/liolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/llex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lmem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/loadlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lobject.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lopcodes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/loslib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lparser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lpico8lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstring.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstrlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltable.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltablib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltests.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lundump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lvm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lzio.c)

if (BUILD_FOR_NOKIA_NGAGE)
  if(DEFINED ENV{NGAGESDK})
    set(NGAGESDK $ENV{NGAGESDK})
    set(CMAKE_TOOLCHAIN_FILE ${NGAGESDK}/cmake/ngage-toolchain.gcc29.cmake)
  else()
    message(FATAL_ERROR "The environment variable NGAGESDK needs to be defined.")
  endif()

  project(z8lua C)

  set(GCC_MODULE_COMN_DEFS -D__SYMBIAN32__ -D__GCC32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARMI__)
  set(GCC_MODULE_MODE_DEFS -DNDEBUG -D_UNICODE)
  set(GCC_MODULE_DEFS      ${GCC_MODULE_COMN_DEFS} ${GCC_MODULE_MODE_DEFS})

  set(z8lua_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/lapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lauxlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lbaselib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lbitlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lcode.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lcorolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lctype.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldblib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldebug.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ldump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lfunc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lgc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/linit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/liolib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/llex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/loadlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lobject.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lopcodes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/loslib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lparser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lpico8lib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lstate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lstring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lstrlib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltablib.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltests.c
    ${CMAKE_CURRENT_SOURCE_DIR}/ltm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lundump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lzio.c)

  add_library(z8lua STATIC ${z8lua_sources})

  target_compile_definitions(
    z8lua
    PUBLIC
    ${GCC_MODULE_DEFS})

  target_compile_options(
    z8lua
    PUBLIC
    -O3)

else()

  project(z8lua C)
  add_executable(z8lua ${z8lua_sources} ${CMAKE_CURRENT_SOURCE_DIR}/lua.c)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

endif()
