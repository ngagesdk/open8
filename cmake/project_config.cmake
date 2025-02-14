# Project configuration.

set(project_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/api.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/emulator.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/image_loader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/p8_compress.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/pxa_compress_snippets.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lapi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lauxlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lbaselib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lbitlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lcode.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lcorolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lctype.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldblib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldebug.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ldump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lfunc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lgc.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/linit.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/liolib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/llex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lmathlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lmem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/loadlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lobject.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lopcodes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/loslib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lparser.c
  #${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lpico8lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstring.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstrlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltable.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltablib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltests.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltm.c
  #${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lua.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lundump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lvm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lzio.c)

set(UID3 0x1000c37e) # game.exe UID
set(APP_UID 0x100051C0) # Pico-8.app UID
set(APP_NAME "Pico-8")
