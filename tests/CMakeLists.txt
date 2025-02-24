cmake_minimum_required(VERSION 3.14)
project(tests C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_C_STANDARD 99)

include(FetchContent)

FetchContent_Declare(
    sdl3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.4/SDL3-devel-3.2.4-VC.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

FetchContent_MakeAvailable(sdl3)
message(STATUS "Extracted files are located in: ${sdl3_SOURCE_DIR}")

set(PICO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src")

set(base_sources
  ${PICO_DIR}/api.c
  ${PICO_DIR}/z8lua/lapi.c
  ${PICO_DIR}/z8lua/lauxlib.c
  ${PICO_DIR}/z8lua/lbaselib.c
  ${PICO_DIR}/z8lua/lcode.c
  ${PICO_DIR}/z8lua/lcorolib.c
  ${PICO_DIR}/z8lua/lctype.c
  ${PICO_DIR}/z8lua/ldblib.c
  ${PICO_DIR}/z8lua/ldebug.c
  ${PICO_DIR}/z8lua/ldo.c
  ${PICO_DIR}/z8lua/ldump.c
  ${PICO_DIR}/z8lua/lfunc.c
  ${PICO_DIR}/z8lua/lgc.c
  ${PICO_DIR}/z8lua/linit.c
  ${PICO_DIR}/z8lua/llex.c
  ${PICO_DIR}/z8lua/lmem.c
  ${PICO_DIR}/z8lua/lobject.c
  ${PICO_DIR}/z8lua/lopcodes.c
  ${PICO_DIR}/z8lua/lparser.c
  ${PICO_DIR}/z8lua/lpico8lib.c
  ${PICO_DIR}/z8lua/lstate.c
  ${PICO_DIR}/z8lua/lstring.c
  ${PICO_DIR}/z8lua/lstrlib.c
  ${PICO_DIR}/z8lua/ltable.c
  ${PICO_DIR}/z8lua/ltablib.c
  ${PICO_DIR}/z8lua/ltm.c
  ${PICO_DIR}/z8lua/lundump.c
  ${PICO_DIR}/z8lua/lvm.c
  ${PICO_DIR}/z8lua/lzio.c)

set(tests_sources ${CMAKE_CURRENT_SOURCE_DIR}/tests.c)
set(graphics_sources ${CMAKE_CURRENT_SOURCE_DIR}/graphics.c)

add_library(SDL3 STATIC IMPORTED)

set_target_properties(SDL3 PROPERTIES
  IMPORTED_LOCATION "${sdl3_SOURCE_DIR}/lib/x86/SDL3.lib")

add_executable(tests ${base_sources} ${tests_sources})
target_link_libraries(tests PRIVATE SDL3)

add_executable(graphics WIN32 ${base_sources} ${graphics_sources})
target_link_libraries(graphics PRIVATE SDL3)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

configure_file("${sdl3_SOURCE_DIR}/lib/x86/SDL3.dll" ${CMAKE_CURRENT_SOURCE_DIR} COPYONLY)

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}/../src"
  "${sdl3_SOURCE_DIR}/include")
