# Do not modify this file unless absolutely necessary.
# All project-specific settings can be found in the file:
# project_config.cmake

cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(open8 C)

if (NGAGESDK)
  find_package(SDL3 REQUIRED)
  find_package(SDL3_mixer REQUIRED)
else()
  set(EXPORT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/export)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXPORT_DIR})
  include(get_SDL3)
  get_SDL3("3.2.12")
endif()

set(open8_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/api.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/auxiliary.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/memory.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/p8scii.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/p8_compress.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lexaloffle/pxa_compress_snippets.c)

set(z8lua_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lapi.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lauxlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lbaselib.c
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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/llex.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lmem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lobject.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lopcodes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lparser.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lpico8lib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstate.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstring.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lstrlib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltable.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltablib.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/ltm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lundump.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lvm.c
  ${CMAKE_CURRENT_SOURCE_DIR}/src/z8lua/lzio.c)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_executable(open8 ${open8_sources} ${z8lua_sources})
else()
  add_executable(open8 WIN32 ${open8_sources} ${z8lua_sources})
endif()

if (NGAGESDK)
  target_link_libraries(open8 PRIVATE SDL3_mixer::SDL3_mixer)
  target_link_libraries(open8 PRIVATE SDL3::SDL3)

  set_property(TARGET open8 PROPERTY C_STANDARD 99)

  target_link_options(open8 PRIVATE "SHELL:-s UID1=0x1000007a") # KExecutableImageUidValue, e32uid.h
  target_link_options(open8 PRIVATE "SHELL:-s UID2=0x100039ce") # KAppUidValue16, apadef.h
  target_link_options(open8 PRIVATE "SHELL:-s UID3=0x1000c37e") # open8.exe UID
else()
  target_link_libraries(open8 PRIVATE ${SDL3_LIBRARIES})
endif()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${sdl3_SOURCE_DIR}/include")

if (NINTENDO_3DS)
  set(ROMFS_DIR "${CMAKE_CURRENT_BINARY_DIR}/romfs")
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/export/carts DESTINATION ${ROMFS_DIR})
  set(SMDH_FILE "${CMAKE_CURRENT_BINARY_DIR}/open8.smdh")
  ctr_generate_smdh("${SMDH_FILE}"
    NAME "open8"
    DESCRIPTION "A portable PICO-8 emulator"
    AUTHOR "Michael Fitzmayer"
    ICON "${CMAKE_CURRENT_SOURCE_DIR}/icon.png")
  ctr_create_3dsx(
    open8
    ROMFS "${ROMFS_DIR}"
    SMDH "${SMDH_FILE}")
endif()
