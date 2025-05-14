cmake_minimum_required(VERSION 3.10)

set(z8lua_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/lapi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lauxlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lbaselib.c
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
  ${CMAKE_CURRENT_SOURCE_DIR}/llex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lmem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lobject.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lopcodes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lparser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lpico8lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstring.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lstrlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltable.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltablib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ltm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lundump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lvm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/lzio.c)

project(z8lua C)

if(NGAGESDK)

  add_library(z8lua STATIC ${z8lua_sources})

else()

  project(z8lua C)
  add_library(z8lua STATIC ${z8lua_sources})
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

endif()
